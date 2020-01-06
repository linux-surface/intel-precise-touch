#include <linux/circ_buf.h>
#include <linux/ioctl.h>
#include <linux/poll.h>
#include <linux/rculist.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "uapi.h"


#define IPTS_UAPI_CLIENT_GET_DEVINFO	_IOR(0x86, 0x01, struct ipts_device_info)
#define IPTS_UAPI_CLIENT_START		_IO(0x86, 0x02)
#define IPTS_UAPI_CLIENT_STOP		_IO(0x86, 0x03)


struct ipts_uapi_client {
	struct list_head node;
	struct ipts_uapi_device *uapi_dev;
	struct fasync_struct *fasync;

	atomic_t active;
	struct {
		u32 capacity;
		u32 head;
		u32 tail;
		u8 *data;
	} buffer;
};


inline u32 next_power_of_two(u32 x)
{
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;

	return x;
}


void ipts_uapi_push_client(struct ipts_uapi_client *client, const void *data, u32 size)
{
	struct device *dev = client->uapi_dev->ctx->dev;
	u32 head = client->buffer.head;
	u32 tail = smp_load_acquire(&client->buffer.tail);
	u32 m;

	if (!atomic_read(&client->active))
		return;

	if (CIRC_SPACE(head, tail, client->buffer.capacity) < size) {
		dev_warn(dev, "client dropping data frame\n");
		return;
	}

	if (head + size < client->buffer.capacity) {
		memcpy(client->buffer.data + head, data, size);
	} else {
		m = CIRC_SPACE_TO_END(head, tail, client->buffer.capacity);
		memcpy(client->buffer.data + head, data, m);
		memcpy(client->buffer.data, data + m, size - m);
	}

	smp_store_release(&client->buffer.head, (head + size) & (client->buffer.capacity - 1));
	kill_fasync(&client->fasync, SIGIO, POLL_IN);
}

void ipts_uapi_push(struct ipts_uapi_device *dev, const void *data, u32 size)
{
	struct ipts_uapi_client *client;

	rcu_read_lock();
	list_for_each_entry_rcu(client, &dev->client_list, node)
		ipts_uapi_push_client(client, data, size);
	rcu_read_unlock();

	wake_up_interruptible(&dev->waitq);
}


static int ipts_uapi_client_open(struct inode *inode, struct file *file)
{
	struct ipts_uapi_device *dev = container_of(file->private_data, struct ipts_uapi_device, mdev);
	struct ipts_uapi_client *client;

	// FIXME: make ipts_context::status atomic?
	if (dev->ctx->status < IPTS_HOST_STATUS_RESOURCE_READY)
		return -ENODEV;

	client = kzalloc(sizeof(struct ipts_uapi_client), GFP_KERNEL);
	if (!client)
		return -ENOMEM;

	client->uapi_dev = dev;
	atomic_set(&client->active, false);

	client->buffer.capacity = next_power_of_two(dev->ctx->device_info.frame_size * 16);
	client->buffer.head = 0;
	client->buffer.tail = 0;
	client->buffer.data = kzalloc(client->buffer.capacity, GFP_KERNEL);

	if (!client->buffer.data) {
		kfree(client);
		return -ENOMEM;
	}

	// attach client
	spin_lock(&dev->client_lock);
	list_add_tail_rcu(&client->node, &dev->client_list);
	spin_unlock(&dev->client_lock);

	file->private_data = client;
	nonseekable_open(inode, file);

	return 0;
}

static int ipts_uapi_client_release(struct inode *inode, struct file *file)
{
	struct ipts_uapi_client *client = file->private_data;

	// detach client
	spin_lock(&client->uapi_dev->client_lock);
	list_del_rcu(&client->node);
	spin_unlock(&client->uapi_dev->client_lock);
	synchronize_rcu();

	kfree(client->buffer.data);
	kfree(client);

	file->private_data = NULL;

	return 0;
}

static ssize_t ipts_uapi_client_read(struct file *file, char __user *buf, size_t count, loff_t *offs)
{
	struct ipts_uapi_client *client = file->private_data;
	u32 head = smp_load_acquire(&client->buffer.head);
	u32 tail = client->buffer.tail;
	u32 n, m;
	int status;

	if (client->uapi_dev->ctx->status < IPTS_HOST_STATUS_RESOURCE_READY)
		return -ENODEV;

	if (CIRC_CNT(head, tail, client->buffer.capacity) == 0) {
		status = wait_event_interruptible(client->uapi_dev->waitq,
				smp_load_acquire(&client->buffer.head) != tail
				|| client->uapi_dev->ctx->status
					< IPTS_HOST_STATUS_RESOURCE_READY);
		if (status) {
			return status;
		}

		if (client->uapi_dev->ctx->status < IPTS_HOST_STATUS_RESOURCE_READY)
			return -ENODEV;

		head = smp_load_acquire(&client->buffer.head);
	}

	n = CIRC_CNT(head, tail, client->buffer.capacity);
	n = n <= count ? n : count;

	if (tail + n < client->buffer.capacity) {
		if (copy_to_user(buf, client->buffer.data + tail, n))
			return -EFAULT;
	} else {
		m = CIRC_CNT_TO_END(head, tail, client->buffer.capacity);
		if (copy_to_user(buf, client->buffer.data + tail, m))
			return -EFAULT;
		if (copy_to_user(buf + m, client->buffer.data, n - m))
			return -EFAULT;
	}

	smp_store_release(&client->buffer.tail, (tail + n) & (client->buffer.capacity - 1));
	return n;
}

static __poll_t ipts_uapi_client_poll(struct file *file, struct poll_table_struct *pt)
{
	struct ipts_uapi_client *client = file->private_data;

	// FIXME: make ipts_context::status atomic?
	if (client->uapi_dev->ctx->status < IPTS_HOST_STATUS_RESOURCE_READY)
		return EPOLLHUP | EPOLLERR;

	if (smp_load_acquire(&client->buffer.head) != smp_load_acquire(&client->buffer.tail))
		return EPOLLIN | EPOLLRDNORM;

	return 0;
}

static int ipts_uapi_client_fasync(int fd, struct file *file, int on)
{
	struct ipts_uapi_client *client = file->private_data;
	return fasync_helper(fd, file, on, &client->fasync);
}

static long ipts_uapi_client_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct ipts_uapi_client *client = file->private_data;
	struct ipts_context *ctx = client->uapi_dev->ctx;
	int status = 0;

	switch (cmd) {
	case IPTS_UAPI_CLIENT_GET_DEVINFO:
		if (copy_to_user((void __user *)arg, &ctx->device_info,
				 sizeof(struct ipts_device_info)))
			status = -EFAULT;
		break;

	case IPTS_UAPI_CLIENT_START:
		atomic_set(&client->active, true);
		break;

	case IPTS_UAPI_CLIENT_STOP:
		atomic_set(&client->active, false);
		break;

	default:
		status = -EINVAL;
		break;
	}

	return status;
}

static const struct file_operations ipts_uapi_fops = {
	.owner          = THIS_MODULE,
	.open           = ipts_uapi_client_open,
	.release        = ipts_uapi_client_release,
	.read           = ipts_uapi_client_read,
	.poll           = ipts_uapi_client_poll,
	.fasync         = ipts_uapi_client_fasync,
	.unlocked_ioctl = ipts_uapi_client_ioctl,
	.llseek         = no_llseek,
};


struct ipts_uapi_device *ipts_uapi_device_init(struct ipts_context *ctx)
{
	struct ipts_uapi_device *dev;
	int status;

	dev = kzalloc(sizeof(struct ipts_uapi_device), GFP_KERNEL);
	if (!dev)
		return ERR_PTR(-ENOMEM);

	dev->ctx = ctx;

	dev->mdev.name = "ipts";
	dev->mdev.minor = MISC_DYNAMIC_MINOR;
	dev->mdev.fops = &ipts_uapi_fops;

	spin_lock_init(&dev->client_lock);
	INIT_LIST_HEAD(&dev->client_list);
	init_waitqueue_head(&dev->waitq);

	status = misc_register(&dev->mdev);
	if (status)
		return ERR_PTR(status);

	return dev;
}

void ipts_uapi_device_free(struct ipts_uapi_device *dev)
{
	misc_deregister(&dev->mdev);
	kfree(dev);
}

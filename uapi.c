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
};


static int ipts_uapi_client_open(struct inode *inode, struct file *file)
{
	struct ipts_uapi_device *dev = container_of(file->private_data, struct ipts_uapi_device, mdev);
	struct ipts_uapi_client *client;

	client = kzalloc(sizeof(struct ipts_uapi_client), GFP_KERNEL);
	if (!client)
		return -ENOMEM;

	client->uapi_dev = dev;

	// TODO

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

	// TODO

	kfree(client);
	file->private_data = NULL;

	return 0;
}

static ssize_t ipts_uapi_client_read(struct file *file, char __user *buf, size_t count, loff_t *offs)
{
	return 0;	// TODO
}

static __poll_t ipts_uapi_client_poll(struct file *file, struct poll_table_struct *pt)
{
	return 0;	// TODO
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

	if (ctx->status < IPTS_HOST_STATUS_RESOURCE_READY)
		return -ENODEV;

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

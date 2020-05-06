// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/ioctl.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include "context.h"
#include "control.h"

#define IPTS_UAPI_INFO  _IOR(0x86, 0x01, struct ipts_device_info)
#define IPTS_UAPI_START _IO(0x86, 0x02)
#define IPTS_UAPI_STOP  _IO(0x86, 0x03)

struct ipts_uapi_client {
	struct ipts_context *ipts;
	bool active;
	u32 offset;
};

static int ipts_uapi_open(struct inode *inode, struct file *file)
{
	struct ipts_uapi_client *client;

	struct ipts_uapi *uapi = container_of(file->private_data,
			struct ipts_uapi, device);
	struct ipts_context *ipts = container_of(uapi,
			struct ipts_context, uapi);

	if (ipts->status != IPTS_HOST_STATUS_STARTED)
		return -ENODEV;

	client = kzalloc(sizeof(struct ipts_uapi_client), GFP_KERNEL);
	if (!client)
		return -ENOMEM;

	client->ipts = ipts;

	file->private_data = client;
	nonseekable_open(inode, file);

	return 0;
}

static int ipts_uapi_close(struct inode *inode, struct file *file)
{
	struct ipts_uapi_client *client = file->private_data;
	struct ipts_context *ipts = client->ipts;

	if (client->active)
		ipts->uapi.active = false;

	kfree(client);
	file->private_data = NULL;

	return 0;
}

static ssize_t ipts_uapi_read(struct file *file, char __user *buffer,
		size_t count, loff_t *offs)
{
	u32 available;
	u32 to_read;

	char *data;
	u8 buffer_id;

	struct ipts_uapi_client *client = file->private_data;
	struct ipts_context *ipts = client->ipts;
	u32 *doorbell = (u32 *)ipts->doorbell.address;

	if (ipts->status != IPTS_HOST_STATUS_STARTED)
		return -ENODEV;

	if (!client->active)
		return 0;

	available = ipts->device_info.data_size - client->offset;
	to_read = available < count ? available : count;

	if (ipts->uapi.doorbell == *doorbell)
		return 0;

	buffer_id = ipts->uapi.doorbell % 16;
	data = ipts->data[buffer_id].address;

	if (copy_to_user(buffer, data + client->offset, to_read))
		return -EFAULT;

	client->offset += to_read;
	if (client->offset < ipts->device_info.data_size)
		return to_read;

	client->offset = 0;
	ipts->uapi.doorbell++;

	if (ipts_control_send_feedback(ipts, buffer_id, data[64]))
		return -EFAULT;

	return to_read;
}

static long ipts_uapi_ioctl_info(struct ipts_uapi_client *client,
		unsigned long arg)
{
	int ret;
	void __user *buffer = (void __user *)arg;
	struct ipts_context *ipts = client->ipts;

	ret = copy_to_user(buffer, &ipts->device_info,
			sizeof(struct ipts_device_info));
	if (ret)
		return -EFAULT;

	return 0;
}

static long ipts_uapi_ioctl_start(struct ipts_uapi_client *client)
{
	struct ipts_context *ipts = client->ipts;

	if (ipts->uapi.active || client->active)
		return -EFAULT;

	ipts->uapi.active = true;
	client->active = true;

	return 0;
}

static long ipts_uapi_ioctl_stop(struct ipts_uapi_client *client)
{
	struct ipts_context *ipts = client->ipts;

	if (!ipts->uapi.active || !client->active)
		return -EFAULT;

	ipts->uapi.active = false;
	client->active = false;

	return 0;
}

static long ipts_uapi_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	struct ipts_uapi_client *client = file->private_data;

	switch (cmd) {
	case IPTS_UAPI_INFO:
		return ipts_uapi_ioctl_info(client, arg);
	case IPTS_UAPI_START:
		return ipts_uapi_ioctl_start(client);
	case IPTS_UAPI_STOP:
		return ipts_uapi_ioctl_stop(client);
	default:
		return -EINVAL;
	}
}

static const struct file_operations ipts_uapi_fops = {
	.owner = THIS_MODULE,
	.open = ipts_uapi_open,
	.release = ipts_uapi_close,
	.read = ipts_uapi_read,
	.unlocked_ioctl = ipts_uapi_ioctl,
	.llseek = no_llseek,
};

int ipts_uapi_init(struct ipts_context *ipts)
{
	ipts->uapi.device.name = "ipts";
	ipts->uapi.device.minor = MISC_DYNAMIC_MINOR;
	ipts->uapi.device.fops = &ipts_uapi_fops;

	return misc_register(&ipts->uapi.device);
}

void ipts_uapi_free(struct ipts_context *ipts)
{
	misc_deregister(&ipts->uapi.device);
}



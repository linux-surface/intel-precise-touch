// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/types.h>

#include "context.h"
#include "control.h"
#include "protocol.h"

struct ipts_device_info {
	__u16 vendor;
	__u16 product;
	__u32 version;
	__u32 buffer_size;
	__u8 max_contacts;

	/* For future expansion */
	__u8 reserved[19];
};

#define IPTS_IOCTL_GET_DEVICE_INFO _IOR(0x86, 0x01, struct ipts_device_info)
#define IPTS_IOCTL_GET_DOORBELL    _IOR(0x86, 0x02, __u32)
#define IPTS_IOCTL_SEND_FEEDBACK   _IO(0x86, 0x03)

static int ipts_uapi_open(struct inode *inode, struct file *file)
{
	struct ipts_chardev *cdev = container_of(inode->i_cdev,
			struct ipts_chardev, chardev);

	file->private_data = cdev->ipts;

	return 0;
}

static ssize_t ipts_uapi_read(struct file *file, char __user *buf,
		size_t count, loff_t *offset)
{
	int buffer;
	int maxbytes;
	struct ipts_context *ipts = file->private_data;

	buffer = MINOR(file->f_path.dentry->d_inode->i_rdev);

	if (!ipts->data[buffer].address)
		return -ENODEV;

	maxbytes = ipts->device_info.data_size - *offset;
	if (maxbytes <= 0)
		return -EINVAL;

	if (count > maxbytes)
		return -EINVAL;

	if (copy_to_user(buf, ipts->data[buffer].address + *offset, count))
		return -EFAULT;

	return count;
}

static long ipts_uapi_ioctl_get_device_info(struct ipts_context *ipts,
		unsigned long arg)
{
	struct ipts_device_info info;
	void __user *buffer = (void __user *)arg;

	info.vendor = ipts->device_info.vendor_id;
	info.product = ipts->device_info.device_id;
	info.version = ipts->device_info.fw_rev;
	info.buffer_size = ipts->device_info.data_size;
	info.max_contacts = ipts->device_info.max_contacts;

	if (copy_to_user(buffer, &info, sizeof(struct ipts_device_info)))
		return -EFAULT;

	return 0;
}

static long ipts_uapi_ioctl_get_doorbell(struct ipts_context *ipts,
		unsigned long arg)
{
	void __user *buffer = (void __user *)arg;

	if (!ipts->doorbell.address)
		return -ENODEV;

	if (copy_to_user(buffer, ipts->doorbell.address, sizeof(u32)))
		return -EFAULT;

	return 0;
}

static long ipts_uapi_ioctl_send_feedback(struct ipts_context *ipts,
		struct file *file)
{
	int ret;
	struct ipts_feedback_cmd cmd;

	memset(&cmd, 0, sizeof(struct ipts_feedback_cmd));
	cmd.buffer = MINOR(file->f_path.dentry->d_inode->i_rdev);

	ret = ipts_control_send(ipts, IPTS_CMD_FEEDBACK,
				&cmd, sizeof(struct ipts_feedback_cmd));

	if (ret)
		return -EFAULT;

	return 0;
}

static long ipts_uapi_ioctl(struct file *file, unsigned int cmd,
		unsigned long arg)
{
	struct ipts_context *ipts = file->private_data;

	switch (cmd) {
	case IPTS_IOCTL_GET_DEVICE_INFO:
		return ipts_uapi_ioctl_get_device_info(ipts, arg);
	case IPTS_IOCTL_GET_DOORBELL:
		return ipts_uapi_ioctl_get_doorbell(ipts, arg);
	case IPTS_IOCTL_SEND_FEEDBACK:
		return ipts_uapi_ioctl_send_feedback(ipts, file);
	default:
		return -ENOTTY;
	}
}

static const struct file_operations ipts_uapi_fops = {
	.owner = THIS_MODULE,
	.open = ipts_uapi_open,
	.read = ipts_uapi_read,
	.unlocked_ioctl = ipts_uapi_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = ipts_uapi_ioctl,
#endif
};

int ipts_uapi_init(struct ipts_context *ipts)
{
	int i, major;

	alloc_chrdev_region(&ipts->uapi.device, 0, IPTS_BUFFERS, "ipts");
	ipts->uapi.class = class_create(THIS_MODULE, "ipts");

	major = MAJOR(ipts->uapi.device);

	for (i = 0; i < IPTS_BUFFERS; i++) {
		cdev_init(&ipts->uapi.cdevs[i].chardev, &ipts_uapi_fops);

		ipts->uapi.cdevs[i].ipts = ipts;
		ipts->uapi.cdevs[i].chardev.owner = THIS_MODULE;

		cdev_add(&ipts->uapi.cdevs[i].chardev, MKDEV(major, i), 1);

		device_create(ipts->uapi.class, NULL,
				MKDEV(major, i), NULL, "ipts/%d", i);
	}

	return 0;
}

void ipts_uapi_free(struct ipts_context *ipts)
{
	int i;
	int major;

	major = MAJOR(ipts->uapi.device);

	for (i = 0; i < IPTS_BUFFERS; i++)
		device_destroy(ipts->uapi.class, MKDEV(major, i));

	class_unregister(ipts->uapi.class);
	class_destroy(ipts->uapi.class);

	unregister_chrdev_region(MKDEV(major, 0), MINORMASK);
}


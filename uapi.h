// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _IPTS_UAPI_H_
#define _IPTS_UAPI_H_

#include "context.h"


struct ipts_uapi_device {
	struct ipts_context *ctx;
	struct miscdevice mdev;

	spinlock_t client_lock;
	struct list_head client_list;

	wait_queue_head_t waitq;
};


struct ipts_uapi_device *ipts_uapi_device_init(struct ipts_context *ctx);
void ipts_uapi_device_free(struct ipts_uapi_device *dev);
void ipts_uapi_push(struct ipts_uapi_device *dev, const void *data, u32 size);

#endif /* _IPTS_UAPI_H */

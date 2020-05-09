/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTS_CONTEXT_H_
#define _IPTS_CONTEXT_H_

#include <linux/kthread.h>
#include <linux/mei_cl_bus.h>
#include <linux/miscdevice.h>
#include <linux/types.h>

#include "protocol/responses.h"

/* IPTS driver states */
enum ipts_host_status {
	IPTS_HOST_STATUS_STOPPED,
	IPTS_HOST_STATUS_STARTING,
	IPTS_HOST_STATUS_STARTED,
	IPTS_HOST_STATUS_RESTARTING
};

struct ipts_buffer_info {
	u8 *address;
	dma_addr_t dma_address;
};

struct ipts_uapi {
	struct miscdevice device;
	struct task_struct *db_thread;

	u32 doorbell;
	bool active;
};

struct ipts_context {
	struct mei_cl_device *cldev;
	struct device *dev;
	struct ipts_device_info device_info;

	enum ipts_host_status status;

	struct ipts_buffer_info data[16];
	struct ipts_buffer_info feedback[16];
	struct ipts_buffer_info doorbell;

	/*
	 * These buffers are not actually used by anything, but they need
	 * to be allocated and passed to the ME to get proper functionality.
	 */
	struct ipts_buffer_info workqueue;
	struct ipts_buffer_info host2me;

	struct ipts_uapi uapi;
};

#endif /* _IPTS_CONTEXT_H_ */

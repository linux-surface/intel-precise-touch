/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTS_CONTEXT_H_
#define _IPTS_CONTEXT_H_

#include <linux/kthread.h>
#include <linux/mei_cl_bus.h>
#include <linux/types.h>

#include "protocol/responses.h"

/*
 * IPTS driver states
 */
enum ipts_host_status {
	IPTS_HOST_STATUS_NONE,
	IPTS_HOST_STATUS_INIT,
	IPTS_HOST_STATUS_RESOURCE_READY,
	IPTS_HOST_STATUS_STARTED,
	IPTS_HOST_STATUS_STOPPING
};

struct ipts_buffer_info {
	u32 size;
	u8 *address;
	dma_addr_t dma_address;
};

struct ipts_context {
	struct mei_cl_device *client_dev;
	struct device *dev;
	struct ipts_device_info device_info;

	struct ipts_buffer_info touch_data[16];
	struct ipts_buffer_info feedback[16];
	enum ipts_host_status status;

	struct task_struct *receiver_loop;
};

#endif /* _IPTS_CONTEXT_H_ */

/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTS_CONTEXT_H_
#define _IPTS_CONTEXT_H_

#include <linux/mei_cl_bus.h>
#include <linux/types.h>

struct ipts_buffer_info {
	u32 size;
	u8 *address;
	dma_addr_t dma_address;
};

struct ipts_context {
	struct mei_cl_device *client_dev;
	struct device *dev;

	struct ipts_buffer_info touch_data[16];
	struct ipts_buffer_info feedback[16];
};

#endif /* _IPTS_CONTEXT_H_ */

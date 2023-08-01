/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2020-2023 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#ifndef IPTS_RESOURCES_H
#define IPTS_RESOURCES_H

#include <linux/device.h>
#include <linux/types.h>

#include "spec-device.h"

struct ipts_buffer {
	u8 *address;
	size_t size;
};

struct ipts_dma_buffer {
	u8 *address;
	size_t size;

	dma_addr_t dma_address;
	struct device *dma_device;
};

struct ipts_resources {
	struct ipts_dma_buffer data[IPTS_BUFFERS];
	struct ipts_dma_buffer feedback[IPTS_BUFFERS];

	struct ipts_dma_buffer doorbell;
	struct ipts_dma_buffer workqueue;
	struct ipts_dma_buffer hid2me;

	struct ipts_dma_buffer descriptor;

	// Buffer for synthesizing HID reports
	struct ipts_buffer report;
};

int ipts_resources_init(struct ipts_resources *resources, struct device *dev,
			struct ipts_device_info info);
int ipts_resources_free(struct ipts_resources *resources);

#endif /* IPTS_RESOURCES_H */

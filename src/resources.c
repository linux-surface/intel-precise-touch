// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020-2022 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#include <linux/dma-mapping.h>

#include "context.h"

static int ipts_resources_alloc_buffer(struct ipts_context *ipts,
				       struct ipts_buffer_info *buffer,
				       size_t size)
{
	if (buffer->address)
		return 0;

	buffer->address = dma_alloc_coherent(ipts->dev, size,
					     &buffer->dma_address, GFP_KERNEL);

	if (!buffer->address)
		return -ENOMEM;

	return 0;
}

static void ipts_resources_free_buffer(struct ipts_context *ipts,
				       struct ipts_buffer_info *buffer,
				       size_t size)
{
	if (!buffer->address)
		return;

	dma_free_coherent(ipts->dev, size, buffer->address,
			  buffer->dma_address);

	buffer->address = NULL;
	buffer->dma_address = 0;
}

void ipts_resources_free(struct ipts_context *ipts)
{
	int i;

	u32 data_buffer_size = ipts->device_info.data_size;
	u32 feedback_buffer_size = ipts->device_info.feedback_size;

	for (i = 0; i < IPTS_BUFFERS; i++)
		ipts_resources_free_buffer(ipts, &ipts->data[i],
					   data_buffer_size);

	for (i = 0; i < IPTS_BUFFERS; i++)
		ipts_resources_free_buffer(ipts, &ipts->feedback[i],
					   feedback_buffer_size);

	ipts_resources_free_buffer(ipts, &ipts->doorbell, sizeof(u32));
	ipts_resources_free_buffer(ipts, &ipts->workqueue, sizeof(u32));
	ipts_resources_free_buffer(ipts, &ipts->hid2me, feedback_buffer_size);
}

int ipts_resources_alloc(struct ipts_context *ipts)
{
	int i;
	int ret;

	u32 data_buffer_size = ipts->device_info.data_size;
	u32 feedback_buffer_size = ipts->device_info.feedback_size;

	for (i = 0; i < IPTS_BUFFERS; i++) {
		ret = ipts_resources_alloc_buffer(ipts, &ipts->data[i],
						  data_buffer_size);
		if (ret)
			goto err;
	}

	for (i = 0; i < IPTS_BUFFERS; i++) {
		ret = ipts_resources_alloc_buffer(ipts, &ipts->feedback[i],
						  feedback_buffer_size);
		if (ret)
			goto err;
	}

	ret = ipts_resources_alloc_buffer(ipts, &ipts->doorbell, sizeof(u32));
	if (ret)
		goto err;

	ret = ipts_resources_alloc_buffer(ipts, &ipts->workqueue, sizeof(u32));
	if (ret)
		goto err;

	ret = ipts_resources_alloc_buffer(ipts, &ipts->hid2me,
					  feedback_buffer_size);
	if (ret)
		goto err;

	return 0;

err:

	ipts_resources_free(ipts);
	return ret;
}

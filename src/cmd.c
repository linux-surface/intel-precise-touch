// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2022 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#include <linux/mei_cl_bus.h>

#include "context.h"
#include "protocol.h"

int ipts_cmd_send(struct ipts_context *ipts, u32 code, void *payload,
		  size_t size)
{
	int ret;
	struct ipts_command cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.code = code;

	if (payload && size > 0)
		memcpy(cmd.payload, payload, size);

	ret = mei_cldev_send(ipts->cldev, (u8 *)&cmd, sizeof(cmd.code) + size);
	if (ret >= 0)
		return 0;

	dev_err(ipts->dev, "Error while sending: 0x%X:%d\n", code, ret);
	return ret;
}

int ipts_cmd_get_device_info(struct ipts_context *ipts)
{
	return ipts_cmd_send(ipts, IPTS_CMD_GET_DEVICE_INFO, NULL, 0);
}

int ipts_cmd_set_mode(struct ipts_context *ipts, enum ipts_mode mode)
{
	struct ipts_set_mode_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.mode = mode;

	return ipts_cmd_send(ipts, IPTS_CMD_SET_MODE, &cmd, sizeof(cmd));
}

int ipts_cmd_set_mem_window(struct ipts_context *ipts)
{
	int i;
	struct ipts_set_mem_window_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));

	for (i = 0; i < IPTS_BUFFERS; i++) {
		cmd.data_buffer_addr_lower[i] =
			lower_32_bits(ipts->data[i].dma_address);

		cmd.data_buffer_addr_upper[i] =
			upper_32_bits(ipts->data[i].dma_address);

		cmd.feedback_buffer_addr_lower[i] =
			lower_32_bits(ipts->feedback[i].dma_address);

		cmd.feedback_buffer_addr_upper[i] =
			upper_32_bits(ipts->feedback[i].dma_address);
	}

	cmd.workqueue_addr_lower = lower_32_bits(ipts->workqueue.dma_address);
	cmd.workqueue_addr_upper = upper_32_bits(ipts->workqueue.dma_address);

	cmd.doorbell_addr_lower = lower_32_bits(ipts->doorbell.dma_address);
	cmd.doorbell_addr_upper = upper_32_bits(ipts->doorbell.dma_address);

	cmd.hid2me_addr_lower = lower_32_bits(ipts->hid2me.dma_address);
	cmd.hid2me_addr_upper = upper_32_bits(ipts->hid2me.dma_address);

	cmd.workqueue_size = IPTS_WORKQUEUE_SIZE;
	cmd.workqueue_item_size = IPTS_WORKQUEUE_ITEM_SIZE;

	return ipts_cmd_send(ipts, IPTS_CMD_SET_MEM_WINDOW, &cmd, sizeof(cmd));
}

int ipts_cmd_ready_for_data(struct ipts_context *ipts)
{
	return ipts_cmd_send(ipts, IPTS_CMD_READY_FOR_DATA, NULL, 0);
}

int ipts_cmd_feedback(struct ipts_context *ipts, u32 buffer)
{
	struct ipts_feedback_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.buffer = buffer;

	return ipts_cmd_send(ipts, IPTS_CMD_FEEDBACK, &cmd, sizeof(cmd));
}

int ipts_cmd_clear_mem_window(struct ipts_context *ipts)
{
	return ipts_cmd_send(ipts, IPTS_CMD_CLEAR_MEM_WINDOW, NULL, 0);
}

int ipts_cmd_reset(struct ipts_context *ipts, enum ipts_reset_type type)
{
	struct ipts_reset_sensor_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.type = type;

	return ipts_cmd_send(ipts, IPTS_CMD_RESET_SENSOR, &cmd, sizeof(cmd));
}

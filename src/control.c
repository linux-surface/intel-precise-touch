// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020-2023 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#include <linux/delay.h>
#include <linux/dev_printk.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/types.h>

#include "context.h"
#include "control.h"
#include "hid.h"
#include "mei.h"
#include "receiver.h"
#include "resources.h"
#include "spec-dma.h"
#include "spec-mei.h"

static int ipts_control_get_device_info(struct ipts_context *ipts)
{
	int ret = 0;
	struct ipts_response rsp = { 0 };

	ret = ipts_mei_send(&ipts->mei, IPTS_CMD_GET_DEVICE_INFO, NULL, 0);
	if (ret)
		return ret;

	ret = ipts_mei_recv(&ipts->mei, IPTS_CMD_GET_DEVICE_INFO, &rsp);
	if (ret)
		return ret;

	if (rsp.status == IPTS_STATUS_SUCCESS)
		ipts->info = rsp.payload.get_device_info;

	return rsp.status;
}

static int ipts_control_set_mode(struct ipts_context *ipts)
{
	int ret = 0;

	struct ipts_cmd_set_mode cmd = { 0 };
	struct ipts_response rsp = { 0 };

	cmd.mode = ipts->mode;

	ret = ipts_mei_send(&ipts->mei, IPTS_CMD_SET_MODE, &cmd, sizeof(cmd));
	if (ret)
		return ret;

	ret = ipts_mei_recv(&ipts->mei, IPTS_CMD_SET_MODE, &rsp);
	if (ret)
		return ret;

	return rsp.status;
}

static int ipts_control_set_mem_window(struct ipts_context *ipts)
{
	int i = 0;
	int ret = 0;

	struct ipts_cmd_set_mem_window cmd = { 0 };
	struct ipts_response rsp = { 0 };

	for (i = 0; i < IPTS_MAX_BUFFERS; i++) {
		dma_addr_t data_addr = ipts->resources.data[i].dma_address;
		dma_addr_t feedback_addr = ipts->resources.feedback[i].dma_address;

		cmd.data_addr_lower[i] = lower_32_bits(data_addr);
		cmd.data_addr_upper[i] = upper_32_bits(data_addr);

		cmd.feedback_addr_lower[i] = lower_32_bits(feedback_addr);
		cmd.feedback_addr_upper[i] = upper_32_bits(feedback_addr);
	}

	cmd.tail_offset_addr_lower = lower_32_bits(ipts->resources.workqueue.dma_address);
	cmd.tail_offset_addr_lower = upper_32_bits(ipts->resources.workqueue.dma_address);

	cmd.doorbell_addr_lower = lower_32_bits(ipts->resources.doorbell.dma_address);
	cmd.doorbell_addr_upper = upper_32_bits(ipts->resources.doorbell.dma_address);

	cmd.hid2me_addr_lower = lower_32_bits(ipts->resources.hid2me.dma_address);
	cmd.hid2me_addr_upper = upper_32_bits(ipts->resources.hid2me.dma_address);

	cmd.hid2me_size = ipts->resources.hid2me.size;

	cmd.wq_size = IPTS_DEFAULT_WQ_SIZE;
	cmd.wq_item_size = IPTS_DEFAULT_WQ_ITEM_SIZE;

	ret = ipts_mei_send(&ipts->mei, IPTS_CMD_SET_MEM_WINDOW, &cmd, sizeof(cmd));
	if (ret)
		return ret;

	ret = ipts_mei_recv(&ipts->mei, IPTS_CMD_SET_MEM_WINDOW, &rsp);
	if (ret)
		return ret;

	return rsp.status;
}

static int ipts_control_get_descriptor(struct ipts_context *ipts)
{
	int ret = 0;

	struct ipts_cmd_get_hid_desc cmd = { 0 };
	struct ipts_response rsp = { 0 };

	/*
	 * This command is only supported on EDS v2 devices.
	 *
	 * EDS v1 devices without native HID support will use a fallback HID descriptor.
	 */
	if (ipts->info.sensor_eds_intf_rev == 1)
		return 0;

	memset(ipts->resources.descriptor.address, 0, ipts->resources.descriptor.size);

	cmd.addr_lower = lower_32_bits(ipts->resources.descriptor.dma_address);
	cmd.addr_upper = upper_32_bits(ipts->resources.descriptor.dma_address);
	cmd.magic = 8;

	ret = ipts_mei_send(&ipts->mei, IPTS_CMD_GET_HID_DESC, &cmd, sizeof(cmd));
	if (ret)
		return ret;

	ret = ipts_mei_recv(&ipts->mei, IPTS_CMD_GET_HID_DESC, &rsp);
	if (ret)
		return ret;

	return rsp.status;
}

int ipts_control_request_flush(struct ipts_context *ipts)
{
	struct ipts_cmd_quiesce_io cmd = { 0 };

	return ipts_mei_send(&ipts->mei, IPTS_CMD_QUIESCE_IO, &cmd, sizeof(cmd));
}

int ipts_control_wait_flush(struct ipts_context *ipts)
{
	int ret = 0;
	struct ipts_response rsp = { 0 };

	ret = ipts_mei_recv(&ipts->mei, IPTS_CMD_QUIESCE_IO, &rsp);
	if (ret)
		return ret;

	return rsp.status;
}

int ipts_control_request_data(struct ipts_context *ipts)
{
	return ipts_mei_send(&ipts->mei, IPTS_CMD_READY_FOR_DATA, NULL, 0);
}

int ipts_control_wait_data(struct ipts_context *ipts, bool shutdown)
{
	int ret = 0;
	struct ipts_response rsp = { 0 };

	if (!shutdown)
		ret = ipts_mei_recv_timeout(&ipts->mei, IPTS_CMD_READY_FOR_DATA, &rsp, 0);
	else
		ret = ipts_mei_recv(&ipts->mei, IPTS_CMD_READY_FOR_DATA, &rsp);

	if (ret)
		return ret;

	/*
	 * During shutdown, it is possible that the sensor has already been disabled.
	 */
	if (rsp.status == IPTS_STATUS_SENSOR_DISABLED)
		return 0;

	return rsp.status;
}

int ipts_control_send_feedback(struct ipts_context *ipts, u32 buffer_index)
{
	int ret = 0;
	struct ipts_cmd_feedback cmd = { 0 };
	struct ipts_response rsp = { 0 };

	cmd.buffer_index = buffer_index;

	ret = ipts_mei_send(&ipts->mei, IPTS_CMD_FEEDBACK, &cmd, sizeof(cmd));
	if (ret)
		return ret;

	ret = ipts_mei_recv(&ipts->mei, IPTS_CMD_FEEDBACK, &rsp);
	if (ret)
		return ret;

	return rsp.status;
}

int ipts_control_refill_buffer(struct ipts_context *ipts, u32 buffer_index)
{
	int ret = 0;

	/*
	 * To return a buffer to the ME and refill it, the ME expects structured data in the
	 * feedback buffer. Since this format is vendor specific and unknown, we are sending
	 * an empty buffer instead. This successfully refills the buffer, but causes an error that
	 * we are masking here.
	 *
	 * Sending a minimal structure with only the buffer ID fixes the error but breaks refilling
	 * the buffer on some devices.
	 */

	ret = ipts_control_send_feedback(ipts, buffer_index);

	if (ret == IPTS_STATUS_INVALID_PARAMS)
		return 0;

	return ret;
}

int ipts_control_hid2me_feedback(struct ipts_context *ipts, enum ipts_feedback_cmd_type cmd_type,
				 enum ipts_feedback_data_type data_type, void *data, size_t size)
{
	struct ipts_feedback_buffer *buffer = NULL;

	memset(ipts->resources.hid2me.address, 0, ipts->resources.hid2me.size);
	buffer = (struct ipts_feedback_buffer *)ipts->resources.hid2me.address;

	buffer->cmd_type = cmd_type;
	buffer->data_type = data_type;
	buffer->size = size;
	buffer->buffer = IPTS_HID_2_ME_BUFFER_INDEX;

	if (size + sizeof(*buffer) > ipts->resources.hid2me.size)
		return -EINVAL;

	if (data && size > 0)
		memcpy(buffer->data, data, size);

	return ipts_control_send_feedback(ipts, IPTS_HID_2_ME_BUFFER_INDEX);
}

int ipts_control_start(struct ipts_context *ipts)
{
	int ret = 0;

	dev_info(ipts->dev, "Starting IPTS\n");

	ret = ipts_control_get_device_info(ipts);
	if (ret) {
		dev_err(ipts->dev, "Failed to get device info: %d\n", ret);
		return ret;
	}

	dev_info(ipts->dev, "IPTS EDS Version: %d\n", ipts->info.sensor_eds_intf_rev);

	/*
	 * On EDS v2 devices both modes return the same data, so we always initialize the ME
	 * in poll mode. We could stay in event mode, but it has issues with handling large
	 * amounts of data and starts to lag.
	 *
	 * EDS v1 devices have to be initialized in event mode to get fallback singletouch events
	 * until userspace can explicitly turn on raw data once it is ready for processing.
	 */
	if (ipts->info.sensor_eds_intf_rev > 1)
		ipts->mode = IPTS_MODE_POLL;

	ret = ipts_resources_init(&ipts->resources, ipts->dev, ipts->info);
	if (ret) {
		dev_err(ipts->dev, "Failed to allocate buffers: %d", ret);
		return ret;
	}

	ret = ipts_control_get_descriptor(ipts);
	if (ret) {
		dev_err(ipts->dev, "Failed to fetch HID descriptor: %d\n", ret);
		return ret;
	}

	ret = ipts_control_set_mode(ipts);
	if (ret) {
		dev_err(ipts->dev, "Failed to set mode: %d\n", ret);
		return ret;
	}

	ret = ipts_control_set_mem_window(ipts);
	if (ret) {
		dev_err(ipts->dev, "Failed to set memory window: %d\n", ret);
		return ret;
	}

	ret = ipts_receiver_start(ipts);
	if (ret) {
		dev_err(ipts->dev, "Failed to start receiver: %d\n", ret);
		return ret;
	}

	ret = ipts_control_request_data(ipts);
	if (ret) {
		dev_err(ipts->dev, "Failed to request data: %d\n", ret);
		return ret;
	}

	ipts_hid_enable(ipts);

	ret = ipts_hid_init(ipts);
	if (ret) {
		dev_err(ipts->dev, "Failed to initialize HID device: %d\n", ret);
		return ret;
	}

	return 0;
}

static int _ipts_control_stop(struct ipts_context *ipts)
{
	int ret = 0;

	ipts_hid_disable(ipts);
	dev_info(ipts->dev, "Stopping IPTS\n");

	ret = ipts_receiver_stop(ipts);
	if (ret) {
		dev_err(ipts->dev, "Failed to stop receiver: %d\n", ret);
		return ret;
	}

	ret = ipts_resources_free(&ipts->resources);
	if (ret) {
		dev_err(ipts->dev, "Failed to free resources: %d\n", ret);
		return ret;
	}

	return 0;
}

int ipts_control_stop(struct ipts_context *ipts)
{
	int ret = 0;

	ret = _ipts_control_stop(ipts);
	if (ret)
		return ret;

	ret = ipts_hid_free(ipts);
	if (ret) {
		dev_err(ipts->dev, "Failed to free HID device: %d\n", ret);
		return ret;
	}

	return 0;
}

int ipts_control_restart(struct ipts_context *ipts)
{
	int ret = 0;

	ret = _ipts_control_stop(ipts);
	if (ret)
		return ret;

	/*
	 * Wait a second to give the sensor time to fully shut down.
	 */
	msleep(1000);

	ret = ipts_control_start(ipts);
	if (ret)
		return ret;

	return 0;
}

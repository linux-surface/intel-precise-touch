// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020-2022 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#include <linux/mei_cl_bus.h>

#include "cmd.h"
#include "context.h"
#include "protocol.h"

int ipts_control_start(struct ipts_context *ipts)
{
	// The host is already starting or still stopping
	if (ipts->status != IPTS_HOST_STATUS_STOPPED)
		return -EBUSY;

	dev_info(ipts->dev, "Starting IPTS\n");

	// Update host status
	ipts->restart = false;
	ipts->status = IPTS_HOST_STATUS_STARTING;

	// Start initialization by requesting device info
	return ipts_cmd_get_device_info(ipts);
}

int ipts_control_stop(struct ipts_context *ipts)
{
	// The host is already stopping or still starting
	if (ipts->status != IPTS_HOST_STATUS_STARTED)
		return -EBUSY;

	dev_info(ipts->dev, "Stopping IPTS\n");

	// Update host status
	ipts->status = IPTS_HOST_STATUS_STOPPING;

	// Start shutdown by sending feedback for first buffer
	return ipts_cmd_feedback(ipts, 0);
}

int ipts_control_restart(struct ipts_context *ipts)
{
	// Update host status
	ipts->restart = true;

	// Stop the host
	return ipts_control_stop(ipts);
}

int ipts_control_change_mode(struct ipts_context *ipts, enum ipts_mode mode)
{
	if (ipts->mode == mode)
		return 0;

	ipts->mode = mode;
	return ipts_cmd_set_mode(ipts, mode);
}

int ipts_control_hid2me_feedback(struct ipts_context *ipts,
				  enum ipts_feedback_data_type type, u8 *report,
				  size_t size)
{
	struct ipts_feedback_buffer *feedback;

	// Clear the HID2ME buffer
	memset(ipts->hid2me.address, 0, ipts->device_info.feedback_size);
	feedback = (struct ipts_feedback_buffer *)ipts->hid2me.address;

	// Configure the buffer as a SET_FEATURES command
	feedback->cmd_type = IPTS_FEEDBACK_CMD_TYPE_NONE;
	feedback->buffer = IPTS_HID2ME_BUFFER;
	feedback->data_type = type;
	feedback->size = size;

	// Check if the passed report fits into the buffer
	if (size + sizeof(*feedback) > ipts->device_info.feedback_size)
		return -EINVAL;

	memcpy(feedback->payload, report, size);
	return ipts_cmd_feedback(ipts, IPTS_HID2ME_BUFFER);
}

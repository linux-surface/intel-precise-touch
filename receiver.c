// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/types.h>

#include "context.h"
#include "control.h"
#include "protocol/commands.h"
#include "protocol/enums.h"
#include "protocol/events.h"
#include "protocol/responses.h"

static void ipts_receiver_handle_notify_dev_ready(struct ipts_context *ipts,
		struct ipts_response *msg, int *cmd_status)
{
	if (msg->status != IPTS_ME_STATUS_SENSOR_FAIL_NONFATAL &&
			msg->status != IPTS_ME_STATUS_SUCCESS) {
		dev_err(ipts->dev, "0x%08x failed - status = %d\n",
				msg->code, msg->status);
		return;
	}

	*cmd_status = ipts_control_send(ipts,
			IPTS_CMD(GET_DEVICE_INFO), NULL, 0);
}

static void ipts_receiver_handle_get_device_info(struct ipts_context *ipts,
		struct ipts_response *msg, int *cmd_status)
{
	if (msg->status != IPTS_ME_STATUS_COMPAT_CHECK_FAIL &&
			msg->status != IPTS_ME_STATUS_SUCCESS) {
		dev_err(ipts->dev, "0x%08x failed - status = %d\n",
				msg->code, msg->status);
		return;
	}

	memcpy(&ipts->device_info, &msg->data.device_info,
			sizeof(struct ipts_device_info));

	// TODO: Initialize HID

	*cmd_status = ipts_control_send(ipts,
			IPTS_CMD(CLEAR_MEM_WINDOW), NULL, 0);
}

static void ipts_receiver_handle_clear_mem_window(struct ipts_context *ipts,
		struct ipts_response *msg, int *cmd_status, int *ret)
{
	struct ipts_set_mode_cmd sensor_mode_cmd;

	if (msg->status != IPTS_ME_STATUS_TIMEOUT &&
			msg->status != IPTS_ME_STATUS_SUCCESS) {
		dev_err(ipts->dev, "0x%08x failed - status = %d\n",
				msg->code, msg->status);
		return;
	}

	if (ipts->status == IPTS_HOST_STATUS_STOPPING)
		return;

	// TODO: Allocate resources

	ipts->status = IPTS_HOST_STATUS_RESOURCE_READY;

	// We only support multitouch mode at the moment
	memset(&sensor_mode_cmd, 0, sizeof(struct ipts_set_mode_cmd));
	sensor_mode_cmd.sensor_mode = IPTS_SENSOR_MODE_MULTITOUCH;

	*cmd_status = ipts_control_send(ipts, IPTS_CMD(SET_MODE),
			&sensor_mode_cmd, sizeof(struct ipts_set_mode_cmd));
}

static void ipts_receiver_handle_response(struct ipts_context *ipts,
		struct ipts_response *msg, u32 msg_len)
{
	int cmd_status = 0;
	int ret = 0;

	switch (msg->code) {
	case IPTS_RSP(NOTIFY_DEV_READY):
		ipts_receiver_handle_notify_dev_ready(ipts, msg, &cmd_status);
		break;
	case IPTS_RSP(GET_DEVICE_INFO):
		ipts_receiver_handle_get_device_info(ipts, msg, &cmd_status);
		break;
	case IPTS_RSP(CLEAR_MEM_WINDOW):
		ipts_receiver_handle_clear_mem_window(ipts, msg,
				&cmd_status, &ret);
		break;
	}
}

int ipts_receiver_loop(void *data)
{
	u32 msg_len;
	struct ipts_context *ipts;
	struct ipts_response msg;

	ipts = (struct ipts_context *)data;
	dev_dbg(ipts->dev, "Starting receive loop\n");

	while (!kthread_should_stop()) {
		msg_len = mei_cldev_recv(ipts->client_dev,
			(u8 *)&msg, sizeof(msg));

		if (msg_len <= 0) {
			dev_err(ipts->dev, "Error in reading ME message\n");
			continue;
		}

		ipts_receiver_handle_response(ipts, &msg, msg_len);
	}

	dev_dbg(ipts->dev, "Stopping receive loop\n");
	return 0;
}

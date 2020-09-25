// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#include <linux/mei_cl_bus.h>
#include <linux/mutex.h>

#include "context.h"
#include "protocol.h"
#include "resources.h"
#include "uapi.h"

int ipts_control_send(struct ipts_context *ipts,
		u32 code, void *payload, size_t size)
{
	int ret;
	struct ipts_command cmd;

	memset(&cmd, 0, sizeof(struct ipts_command));
	cmd.code = code;

	if (payload && size > 0)
		memcpy(&cmd.payload, payload, size);

	ret = mei_cldev_send(ipts->cldev, (u8 *)&cmd, sizeof(cmd.code) + size);
	if (ret >= 0 || ret == -EINTR)
		return 0;

	dev_err(ipts->dev, "Error while sending: 0x%X:%d\n", code, ret);
	return ret;
}

int ipts_control_start(struct ipts_context *ipts)
{
	dev_info(ipts->dev, "Starting IPTS\n");

	ipts_uapi_init(ipts);
	return ipts_control_send(ipts, IPTS_CMD_GET_DEVICE_INFO, NULL, 0);
}

void ipts_control_restart(struct ipts_context *ipts)
{
	dev_info(ipts->dev, "Stopping IPTS\n");

	ipts_uapi_free(ipts);
	ipts_resources_free(ipts);
	ipts_control_send(ipts, IPTS_CMD_CLEAR_MEM_WINDOW, NULL, 0);
}

void ipts_control_stop(struct ipts_context *ipts)
{
	ipts_control_restart(ipts);
	mei_cldev_disable(ipts->cldev);
}


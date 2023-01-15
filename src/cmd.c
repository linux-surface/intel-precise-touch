// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020-2022 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#include <linux/errno.h>
#include <linux/types.h>

#include "cmd.h"
#include "context.h"
#include "mei.h"
#include "spec-device.h"

int ipts_cmd_recv_timeout(struct ipts_context *ipts, enum ipts_command_code code,
			  struct ipts_response *rsp, int timeout)
{
	int ret;

	if (!ipts)
		return -EFAULT;

	if (!rsp)
		return -EFAULT;

	/*
	 * In a response, the command code will have the most significant bit flipped to 1.
	 * If code is passed to ipts_mei_recv as is, no messages will be recevied.
	 */
	ret = ipts_mei_recv_timeout(&ipts->mei, code | IPTS_RSP_BIT, rsp, timeout);
	if (ret < 0)
		return ret;

	dev_dbg(ipts->dev, "Received 0x%02X with status 0x%02X\n", code, rsp->status);

	/*
	 * Some devices will always return this error.
	 * It is allowed to ignore it and to try continuing.
	 */
	if (rsp->status == IPTS_STATUS_COMPAT_CHECK_FAIL)
		rsp->status = IPTS_STATUS_SUCCESS;

	return 0;
}

int ipts_cmd_recv(struct ipts_context *ipts, enum ipts_command_code code, struct ipts_response *rsp)
{
	return ipts_cmd_recv_timeout(ipts, code, rsp, IPTS_CMD_DEFAULT_TIMEOUT);
}

int ipts_cmd_send(struct ipts_context *ipts, enum ipts_command_code code, void *data, size_t size)
{
	struct ipts_command cmd = { 0 };

	if (!ipts)
		return -EFAULT;

	cmd.cmd = code;

	if (data && size > 0)
		memcpy(cmd.payload, data, size);

	dev_dbg(ipts->dev, "Sending 0x%02X with %ld bytes payload\n", code, size);
	return ipts_mei_send(&ipts->mei, &cmd, sizeof(cmd.cmd) + size);
}

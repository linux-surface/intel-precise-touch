/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020-2022 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#ifndef IPTS_CMD_H
#define IPTS_CMD_H

#include <linux/types.h>

#include "context.h"
#include "spec-device.h"

/*
 * The default timeout for receiving responses
 */
#define IPTS_CMD_DEFAULT_TIMEOUT 1000

/*
 * Receives the response to the given command and copies the payload to the given buffer.
 * This function will block for no longer than the specified timeout in milliseconds. If no
 * data is available, -EAGAIN will be returned. A negative timeout means blocking indefinitly.
 */
int ipts_cmd_recv_timeout(struct ipts_context *ipts, enum ipts_command_code code,
			  struct ipts_response *rsp, int timeout);

/*
 * Receives the response to the given command and copies the payload to the given buffer.
 * This function behaves like ipts_cmd_recv_timeout, but the timeout is specified by
 * IPTS_CMD_DEFAULT_TIMEOUT.
 */
int ipts_cmd_recv(struct ipts_context *ipts, enum ipts_command_code code,
		  struct ipts_response *rsp);

/*
 * Executes the specified command with the given payload on the device.
 */
int ipts_cmd_send(struct ipts_context *ipts, enum ipts_command_code code, void *data, size_t size);

#endif /* IPTS_CMD_H */

/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#ifndef _IPTS_CMD_H_
#define _IPTS_CMD_H_

#include <linux/types.h>

#include "context.h"
#include "protocol.h"

int ipts_cmd_send(struct ipts_context *ipts, u32 cmd, void *payload,
		  size_t size);
int ipts_cmd_get_device_info(struct ipts_context *ipts);
int ipts_cmd_set_mode(struct ipts_context *ipts, enum ipts_mode mode);
int ipts_cmd_set_mem_window(struct ipts_context *ipts);
int ipts_cmd_ready_for_data(struct ipts_context *ipts);
int ipts_cmd_feedback(struct ipts_context *ipts, u32 buffer);
int ipts_cmd_clear_mem_window(struct ipts_context *ipts);
int ipts_cmd_reset(struct ipts_context *ipts, enum ipts_reset_type type);

#endif /* _IPTS_CMD_H_ */

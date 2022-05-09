/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#ifndef _IPTS_CONTROL_H_
#define _IPTS_CONTROL_H_

#include <linux/types.h>

#include "context.h"
#include "protocol.h"

int ipts_control_start(struct ipts_context *ipts);
int ipts_control_restart(struct ipts_context *ipts);
int ipts_control_stop(struct ipts_context *ipts);
int ipts_control_change_mode(struct ipts_context *ipts, enum ipts_mode mode);
int ipts_control_hid2me_feedback(struct ipts_context *ipts,
				  enum ipts_feedback_data_type type, u8 *report,
				  size_t size);

#endif /* _IPTS_CONTROL_H_ */

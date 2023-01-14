/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020-2022 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#ifndef IPTS_MEI_H
#define IPTS_MEI_H

#include <linux/list.h>
#include <linux/mei_cl_bus.h>
#include <linux/rwsem.h>
#include <linux/types.h>
#include <linux/wait.h>

#include "spec-device.h"

struct ipts_mei_message {
	struct list_head list;
	struct ipts_response rsp;
};

struct ipts_mei {
	struct mei_cl_device *cldev;

	struct list_head messages;

	wait_queue_head_t message_queue;
	struct rw_semaphore message_lock;
};

int ipts_mei_recv_timeout(struct ipts_mei *mei, enum ipts_command_code code,
			  struct ipts_response *rsp, int timeout);

int ipts_mei_recv(struct ipts_mei *mei, enum ipts_command_code code, struct ipts_response *rsp);
int ipts_mei_send(struct ipts_mei *mei, void *data, size_t length);

int ipts_mei_init(struct ipts_mei *mei, struct mei_cl_device *cldev);

#endif /* IPTS_MEI_H */

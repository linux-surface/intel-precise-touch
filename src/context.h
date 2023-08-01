/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2020-2023 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#ifndef IPTS_CONTEXT_H
#define IPTS_CONTEXT_H

#include <linux/completion.h>
#include <linux/device.h>
#include <linux/hid.h>
#include <linux/mei_cl_bus.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/types.h>

#include "mei.h"
#include "resources.h"
#include "spec-mei.h"
#include "thread.h"

/**
 * struct ipts_context - Central place to store information about the state of the driver.
 *
 * @dev:
 *     The linux device object of the MEI client device.
 *
 * @mei:
 *     The wrapper for the MEI bus logic.
 *
 * @resources:
 *     The IPTS resource manager, responsible for DMA allocations.
 *
 * @receiver:
 *     The receiver thread that polls the ME for new data or responds to events sent by it.
 *
 * @mode:
 *     The current operating mode of the touch sensor.
 *
 * @info:
 *     Information about the device we are driving.
 *
 * @feature_lock:
 *     Lock that prevents stops userspace from issuing multiple HID_GET_FEATURE requests at the
 *     same time.
 *
 * @feature_event:
 *     The answer to a GET_FEATURE request is sent through a standard IPTS data buffer.
 *     Using this event, the HID interface can wait until the receiver thread has read it.
 *
 * @feature_report:
 *     Once the receiver thread has read the answer to a GET_FEATURE request, it will store
 *     its address in this buffer. Does not own the memory it points to.
 *
 * @hid_active:
 *     Whether the HID interface should be accepting requests from userspace at the moment.
 *
 * @hid:
 *     The linux HID device object.
 */
struct ipts_context {
	struct device *dev;

	struct ipts_mei mei;
	struct ipts_resources resources;
	struct ipts_thread receiver;

	enum ipts_mode mode;
	struct ipts_rsp_get_device_info info;

	struct mutex feature_lock;
	struct completion feature_event;
	struct ipts_buffer feature_report;

	bool hid_active;
	struct hid_device *hid;
};

#endif /* IPTS_CONTEXT_H */

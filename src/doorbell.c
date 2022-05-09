// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2022 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/ktime.h>

#include "cmd.h"
#include "context.h"
#include "hid.h"
#include "protocol.h"

int ipts_doorbell_loop(void *data)
{
	int ret;

	u32 db = 0;
	u32 lastdb = 0;

	struct ipts_context *ipts = data;
	time64_t timeout = ktime_get_seconds() + 5;

	while (!kthread_should_stop()) {
		if (ipts->status != IPTS_HOST_STATUS_STARTED) {
			msleep(1000);
			continue;
		}

		// After filling up one of the data buffers, IPTS will increment
		// the doorbell. The value of the doorbell stands for the *next*
		// buffer that IPTS is going to fill.
		db = *(u32 *)ipts->doorbell.address;
		while (db != lastdb) {
			u32 buffer = lastdb % IPTS_BUFFERS;

			timeout = ktime_get_seconds() + 5;

			ret = ipts_hid_input_data(ipts, buffer);
			if (ret) {
				dev_err(ipts->dev,
					"Failed to send HID report: %d\n", ret);
			}

			ret = ipts_cmd_feedback(ipts, buffer);
			if (ret) {
				dev_err(ipts->dev,
					"Failed to send feedback: %d\n", ret);
			}

			lastdb++;
		}

		// If the last change of the doorbell was less than 5 seconds ago,
		// sleep for a shorter period, so that new data can be processed
		// quickly. If there were no inputs for more than 5 seconds, sleep
		// longer, to avoid wasting CPU cycles.
		if (timeout > ktime_get_seconds())
			msleep(10);
		else
			msleep(200);
	}

	return 0;
}

// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/delay.h>
#include <linux/input.h>
#include <linux/kthread.h>
#include <linux/ktime.h>

#include "context.h"
#include "control.h"
#include "hid.h"
#include "params.h"
#include "protocol/touch.h"

static void ipts_hid_handle_input(struct ipts_context *ipts, int buffer_id)
{
	struct ipts_buffer_info buffer;
	struct ipts_touch_data *data;

	buffer = ipts->touch_data[buffer_id];
	data = (struct ipts_touch_data *)buffer.address;

	if (ipts_params.debug) {
		dev_info(ipts->dev, "Buffer %d\n", buffer_id);
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 32, 1,
				data->data, data->size, false);
	}

	// TODO: forward to user-space

	ipts_control_send_feedback(ipts, buffer_id, data->data[0]);
}

int ipts_hid_loop(void *data)
{
	time64_t ll_timeout;
	u32 doorbell, last_doorbell;
	struct ipts_context *ipts;

	ll_timeout = ktime_get_seconds() + 5;
	ipts = (struct ipts_context *)data;
	last_doorbell = 0;
	doorbell = 0;

	dev_info(ipts->dev, "Starting input loop\n");

	while (!kthread_should_stop()) {
		if (ipts->status != IPTS_HOST_STATUS_STARTED) {
			msleep(1000);
			continue;
		}

		// IPTS will increment the doorbell after it filled up
		// all of the touch data buffers. If the doorbell didn't
		// change, there is no work for us to do.
		doorbell = *(u32 *)ipts->doorbell.address;
		if (doorbell == last_doorbell)
			goto sleep;

		ll_timeout = ktime_get_seconds() + 5;

		while (last_doorbell != doorbell) {
			ipts_hid_handle_input(ipts, last_doorbell % 16);
			last_doorbell++;
		}
sleep:
		if (ll_timeout > ktime_get_seconds())
			usleep_range(5000, 30000);
		else
			msleep(200);
	}

	dev_info(ipts->dev, "Stopping input loop\n");
	return 0;
}

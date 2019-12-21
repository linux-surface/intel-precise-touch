// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/delay.h>
#include <linux/input.h>
#include <linux/kthread.h>

#include "context.h"
#include "control.h"
#include "protocol/touch.h"

// HACK: Workaround for DKMS build without BUS_MEI patch
#ifndef BUS_MEI
#define BUS_MEI 0x44
#endif

static void ipts_hid_handle_input(struct ipts_context *ipts, int buffer_id)
{
	struct ipts_buffer_info buffer;
	struct ipts_touch_data *data;

	buffer = ipts->touch_data[buffer_id];
	data = (struct ipts_touch_data *)buffer.address;

	dev_info(ipts->dev, "Buffer %d\n", data->buffer);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 32, 1,
			data->data, data->size, false);

	ipts_control_send_feedback(ipts, buffer_id, data->data[0]);
}

int ipts_hid_loop(void *data)
{
	u32 doorbell, last_doorbell;
	struct ipts_context *ipts;

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
		if (doorbell == last_doorbell) {
			msleep(100);
			continue;
		}

		dev_info(ipts->dev, "%d\n", doorbell);

		while (last_doorbell != doorbell) {
			ipts_hid_handle_input(ipts, last_doorbell % 16);
			last_doorbell++;
		}

		msleep(100);
	}

	dev_info(ipts->dev, "Stopping input loop\n");
	return 0;
}

static int ipts_hid_create_stylus_input(struct ipts_context *ipts)
{
	int ret;
	char name[64];

	ipts->stylus = devm_input_allocate_device(ipts->dev);
	if (!ipts->stylus)
		return -ENOMEM;

	ipts->stylus_tool = BTN_TOOL_PEN;

	__set_bit(INPUT_PROP_DIRECT, ipts->stylus->propbit);
	__set_bit(INPUT_PROP_POINTER, ipts->stylus->propbit);

	input_set_abs_params(ipts->stylus, ABS_X, 0, 9600, 0, 0);
	input_abs_set_res(ipts->stylus, ABS_X, 34);
	input_set_abs_params(ipts->stylus, ABS_Y, 0, 7200, 0, 0);
	input_abs_set_res(ipts->stylus, ABS_Y, 38);
	input_set_abs_params(ipts->stylus, ABS_PRESSURE, 0, 4096, 0, 0);
	input_set_abs_params(ipts->stylus, ABS_TILT_X, 0, 18000, 0, 0);
	input_abs_set_res(ipts->stylus, ABS_TILT_X, 5730);
	input_set_abs_params(ipts->stylus, ABS_TILT_Y, 0, 18000, 0, 0);
	input_abs_set_res(ipts->stylus, ABS_TILT_Y, 5730);
	input_set_capability(ipts->stylus, EV_KEY, BTN_TOUCH);
	input_set_capability(ipts->stylus, EV_KEY, BTN_STYLUS);
	input_set_capability(ipts->stylus, EV_KEY, BTN_TOOL_PEN);
	input_set_capability(ipts->stylus, EV_KEY, BTN_TOOL_RUBBER);

	ipts->stylus->id.bustype = BUS_MEI;
	ipts->stylus->id.vendor = ipts->device_info.vendor_id;
	ipts->stylus->id.product = ipts->device_info.device_id;
	ipts->stylus->id.version = ipts->device_info.fw_rev;

	snprintf(name, 64, "ipts %04hX:%04hX Stylus",
			ipts->stylus->id.vendor, ipts->stylus->id.product);
	ipts->stylus->phys = "heci3";
	ipts->stylus->name = name;

	ret = input_register_device(ipts->stylus);
	if (ret) {
		dev_err(ipts->dev, "Failed to register stylus device: %d\n",
				ret);
		return ret;
	}

	return 0;
}

int ipts_hid_init(struct ipts_context *ipts)
{
	int ret;

	ret = ipts_hid_create_stylus_input(ipts);
	if (ret)
		return ret;

	return 0;
}

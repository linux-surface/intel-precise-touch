// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/hid.h>
#include <linux/kernel.h>

#include "cmd.h"
#include "context.h"
#include "control.h"
#include "desc.h"
#include "hid.h"
#include "protocol.h"

DECLARE_WAIT_QUEUE_HEAD(wq);

static int ipts_hid_start(struct hid_device *hid)
{
	return 0;
}

static void ipts_hid_stop(struct hid_device *hid)
{
}

static int ipts_hid_open(struct hid_device *hid)
{
	return 0;
}

static void ipts_hid_close(struct hid_device *hid)
{
}

static int ipts_hid_parse(struct hid_device *hid)
{
	int ret;
	struct ipts_context *ipts = hid->driver_data;

	// TODO: Somehow integrate the gen7 0xF command into this
	ret = hid_parse_report(hid, (u8 *)ipts_descriptor,
			       sizeof(ipts_descriptor));
	if (ret) {
		dev_err(ipts->dev, "Failed to parse HID descriptor: %d\n", ret);
		return ret;
	}

	return 0;
}

static int ipts_hid_raw_request(struct hid_device *hid, unsigned char reportnum,
				__u8 *buf, size_t len, unsigned char rtype,
				int reqtype)
{
	enum ipts_feedback_data_type type;
	struct ipts_context *ipts = hid->driver_data;

	// Select the appropreate feedback data type for this report
	if (rtype == HID_OUTPUT_REPORT && reqtype == HID_REQ_SET_REPORT)
		type = IPTS_FEEDBACK_DATA_TYPE_OUTPUT_REPORT;
	else if (rtype == HID_FEATURE_REPORT && reqtype == HID_REQ_GET_REPORT)
		type = IPTS_FEEDBACK_DATA_TYPE_GET_FEATURES;
	else if (rtype == HID_FEATURE_REPORT && reqtype == HID_REQ_SET_REPORT)
		type = IPTS_FEEDBACK_DATA_TYPE_SET_FEATURES;
	else
		return -EIO;

	// Translate the HID command for changing touch modes to the old API
	// that was used before gen7. On gen7, this will only change how the
	// data is received (incremented doorbell or READY_FOR_DATA event).
	if (type == IPTS_FEEDBACK_DATA_TYPE_SET_FEATURES && reportnum == 0x5) {
		int ret;

		ret = ipts_control_change_mode(ipts, buf[1]);
		if (ret) {
			dev_err(ipts->dev, "Failed to switch modes: %d\n", ret);
			return ret;
		}
	}

	ipts->feature_report = NULL;

	// Send Host2ME feedback, containing the report
	ipts_control_host2me_feedback(ipts, type, buf, len);

	// If this was a SET operation, there is no answer to wait for.
	if (reqtype == HID_REQ_SET_REPORT)
		return 0;

	// Wait for an answer to come in
	wait_event(wq, ipts->feature_report);
	memcpy(buf, ipts->feature_report, len);

	ipts->feature_report = NULL;
	return 0;
}

static struct hid_ll_driver ipts_hid_driver = {
	.start = ipts_hid_start,
	.stop = ipts_hid_stop,
	.open = ipts_hid_open,
	.close = ipts_hid_close,
	.parse = ipts_hid_parse,
	.raw_request = ipts_hid_raw_request,
};

int ipts_hid_input_data(struct ipts_context *ipts, int buffer)
{
	int ret;
	u8 report;
	u8 *temp;
	size_t size;
	struct ipts_data *data = (struct ipts_data *)ipts->data[buffer].address;

	// The buffer contains a HID report. Forward as-is.
	if (data->type == IPTS_DATA_TYPE_HID_REPORT)
		return hid_input_report(ipts->hid, HID_INPUT_REPORT, data->data,
					data->size, 1);

	// The data contains the answer to a feature report.
	// Store the data and wake up the HID request handler.
	if (data->type == IPTS_DATA_TYPE_GET_FEATURES) {
		ipts->feature_report = data->data;
		wake_up(&wq);
		return 0;
	}

	if (data->type != IPTS_DATA_TYPE_PAYLOAD)
		return 0;

	// Get the smallest report ID that could fit the data
	report = ipts_desc_get_report(data->size);
	if (report == 0) {
		dev_err(ipts->dev, "Could not find fitting report!\n");
		return -E2BIG;
	}

	// Allocate a buffer for the report
	size = ipts_desc_get_size(report);
	temp = kzalloc(size, GFP_KERNEL);
	if (!temp)
		return -ENOMEM;

	temp[0] = report;
	memcpy(&temp[3], data->data, data->size);

	ret = hid_input_report(ipts->hid, HID_INPUT_REPORT, temp, size, 1);
	kfree(temp);

	return ret;
}

int ipts_hid_init(struct ipts_context *ipts)
{
	int ret;

	ipts->hid = hid_allocate_device();
	if (IS_ERR(ipts->hid)) {
		long err = PTR_ERR(ipts->hid);

		dev_err(ipts->dev, "Failed to allocate HID device: %ld\n", err);
		return err;
	}

	ipts->hid->driver_data = ipts;
	ipts->hid->dev.parent = ipts->dev;
	ipts->hid->ll_driver = &ipts_hid_driver;

	ipts->hid->vendor = ipts->device_info.vendor_id;
	ipts->hid->product = ipts->device_info.device_id;
	ipts->hid->group = HID_GROUP_MULTITOUCH;

	snprintf(ipts->hid->name, sizeof(ipts->hid->name), "IPTS %04X:%04X",
		 ipts->hid->vendor, ipts->hid->product);

	ret = hid_add_device(ipts->hid);
	if (ret) {
		dev_err(ipts->dev, "Failed to add HID device: %d\n", ret);
		hid_destroy_device(ipts->hid);
		return ret;
	}

	return 0;
}

void ipts_hid_free(struct ipts_context *ipts)
{
	hid_destroy_device(ipts->hid);
}

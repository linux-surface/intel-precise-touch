// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2022 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#include <linux/types.h>

#include "desc.h"

u32 ipts_desc_get_size(u8 report)
{
	u32 ret;

	switch (report) {
	case 7:
		ret = IPTS_HID_REPORT_SIZE_7;
		break;
	case 8:
		ret = IPTS_HID_REPORT_SIZE_8;
		break;
	case 10:
		ret = IPTS_HID_REPORT_SIZE_10;
		break;
	case 11:
		ret = IPTS_HID_REPORT_SIZE_11;
		break;
	case 12:
		ret = IPTS_HID_REPORT_SIZE_12;
		break;
	case 13:
		ret = IPTS_HID_REPORT_SIZE_13;
		break;
	case 28:
		ret = IPTS_HID_REPORT_SIZE_28;
		break;
	case 26:
		ret = IPTS_HID_REPORT_SIZE_26;
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

u8 ipts_desc_get_report(u32 size)
{
	// Add HID Header (one byte for report ID, two bytes for timestamp)
	size = size + 3;

	if (size < ipts_desc_get_size(7))
		return 7;

	if (size < ipts_desc_get_size(8))
		return 8;

	if (size < ipts_desc_get_size(10))
		return 10;

	if (size < ipts_desc_get_size(13))
		return 13;

	if (size < ipts_desc_get_size(28))
		return 28;

	if (size < ipts_desc_get_size(11))
		return 11;

	if (size < ipts_desc_get_size(12))
		return 12;

	if (size < ipts_desc_get_size(26))
		return 26;

	return 0;
}

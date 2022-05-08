/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#ifndef _IPTS_DESC_H_
#define _IPTS_DESC_H_

#include <linux/types.h>

#define IPTS_HID_REPORT_SIZE_7	64
#define IPTS_HID_REPORT_SIZE_8	212
#define IPTS_HID_REPORT_SIZE_10 256
#define IPTS_HID_REPORT_SIZE_11 3500
#define IPTS_HID_REPORT_SIZE_12 4300
#define IPTS_HID_REPORT_SIZE_13 1500
#define IPTS_HID_REPORT_SIZE_28 2000
#define IPTS_HID_REPORT_SIZE_26 7488

static const u8 ipts_descriptor[] = {

	// Descriptor for singletouch HID data
	0x05, 0x0D,	  //  Usage Page (Digitizer),
	0x09, 0x04,	  //  Usage (Touchscreen),
	0xA1, 0x01,	  //  Collection (Application),
	0x85, 0x40,	  //      Report ID (64),
	0x09, 0x42,	  //      Usage (Tip Switch),
	0x15, 0x00,	  //      Logical Minimum (0),
	0x25, 0x01,	  //      Logical Maximum (1),
	0x75, 0x01,	  //      Report Size (1),
	0x95, 0x01,	  //      Report Count (1),
	0x81, 0x02,	  //      Input (Variable),
	0x95, 0x07,	  //      Report Count (7),
	0x81, 0x03,	  //      Input (Constant, Variable),
	0x05, 0x01,	  //      Usage Page (Desktop),
	0x09, 0x30,	  //      Usage (X),
	0x75, 0x10,	  //      Report Size (16),
	0x95, 0x01,	  //      Report Count (1),
	0xA4,		  //      Push,
	0x55, 0x0E,	  //      Unit Exponent (14),
	0x65, 0x11,	  //      Unit (Centimeter),
	0x46, 0x76, 0x0B, //      Physical Maximum (2934),
	0x26, 0xFF, 0x7F, //      Logical Maximum (32767),
	0x81, 0x02,	  //      Input (Variable),
	0x09, 0x31,	  //      Usage (Y),
	0x46, 0x74, 0x06, //      Physical Maximum (1652),
	0x26, 0xFF, 0x7F, //      Logical Maximum (32767),
	0x81, 0x02,	  //      Input (Variable),
	0xB4,		  //      Pop,
	0xC0,		  //  End Collection

	// Descriptor for multitouch raw heatmap data
	0x05, 0x0D,	  //  Usage Page (Digitizer),
	0x09, 0x0F,	  //  Usage (Capacitive Hm Digitizer),
	0xA1, 0x01,	  //  Collection (Application),
	0x85, 0x07,	  //      Report ID (7),
	0x09, 0x56,	  //      Usage (Scan Time),
	0x95, 0x01,	  //      Report Count (1),
	0x75, 0x10,	  //      Report Size (16),
	0x81, 0x02,	  //      Input (Variable),
	0x09, 0x61,	  //      Usage (Gesture Char Quality),
	0x75, 0x08,	  //      Report Size (8),
	0x95, 0x3D,	  //      Report Count (61),
	0x81, 0x03,	  //      Input (Constant, Variable),
	0x85, 0x08,	  //      Report ID (8),
	0x09, 0x56,	  //      Usage (Scan Time),
	0x95, 0x01,	  //      Report Count (1),
	0x75, 0x10,	  //      Report Size (16),
	0x81, 0x02,	  //      Input (Variable),
	0x09, 0x61,	  //      Usage (Gesture Char Quality),
	0x75, 0x08,	  //      Report Size (8),
	0x95, 0xD1,	  //      Report Count (209),
	0x81, 0x03,	  //      Input (Constant, Variable),
	0x85, 0x0A,	  //      Report ID (10),
	0x09, 0x56,	  //      Usage (Scan Time),
	0x95, 0x01,	  //      Report Count (1),
	0x75, 0x10,	  //      Report Size (16),
	0x81, 0x02,	  //      Input (Variable),
	0x09, 0x61,	  //      Usage (Gesture Char Quality),
	0x75, 0x08,	  //      Report Size (8),
	0x95, 0xFD,	  //      Report Count (253),
	0x81, 0x03,	  //      Input (Constant, Variable),
	0x85, 0x0B,	  //      Report ID (11),
	0x09, 0x56,	  //      Usage (Scan Time),
	0x95, 0x01,	  //      Report Count (1),
	0x75, 0x10,	  //      Report Size (16),
	0x81, 0x02,	  //      Input (Variable),
	0x09, 0x61,	  //      Usage (Gesture Char Quality),
	0x75, 0x08,	  //      Report Size (8),
	0x96, 0xA9, 0x0D, //      Report Count (3497),
	0x81, 0x03,	  //      Input (Constant, Variable),
	0x85, 0x0C,	  //      Report ID (12),
	0x09, 0x56,	  //      Usage (Scan Time),
	0x95, 0x01,	  //      Report Count (1),
	0x75, 0x10,	  //      Report Size (16),
	0x81, 0x02,	  //      Input (Variable),
	0x09, 0x61,	  //      Usage (Gesture Char Quality),
	0x75, 0x08,	  //      Report Size (8),
	0x96, 0xC9, 0x10, //      Report Count (4297),
	0x81, 0x03,	  //      Input (Constant, Variable),
	0x85, 0x0D,	  //      Report ID (13),
	0x09, 0x56,	  //      Usage (Scan Time),
	0x95, 0x01,	  //      Report Count (1),
	0x75, 0x10,	  //      Report Size (16),
	0x81, 0x02,	  //      Input (Variable),
	0x09, 0x61,	  //      Usage (Gesture Char Quality),
	0x75, 0x08,	  //      Report Size (8),
	0x96, 0xD9, 0x05, //      Report Count (1497),
	0x81, 0x03,	  //      Input (Constant, Variable),
	0x85, 0x1C,	  //      Report ID (28),
	0x09, 0x56,	  //      Usage (Scan Time),
	0x95, 0x01,	  //      Report Count (1),
	0x75, 0x10,	  //      Report Size (16),
	0x81, 0x02,	  //      Input (Variable),
	0x09, 0x61,	  //      Usage (Gesture Char Quality),
	0x75, 0x08,	  //      Report Size (8),
	0x96, 0xCD, 0x07, //      Report Count (1997),
	0x81, 0x03,	  //      Input (Constant, Variable),
	0x85, 0x1A,	  //      Report ID (26),
	0x09, 0x56,	  //      Usage (Scan Time),
	0x95, 0x01,	  //      Report Count (1),
	0x75, 0x10,	  //      Report Size (16),
	0x81, 0x02,	  //      Input (Variable),
	0x09, 0x61,	  //      Usage (Gesture Char Quality),
	0x75, 0x08,	  //      Report Size (8),
	0x96, 0x3D, 0x1D, //      Report Count (7485),
	0x81, 0x03,	  //      Input (Constant, Variable),
	0x85, 0x06,	  //      Report ID (6),
	0x09, 0x63,	  //      Usage (Char Gesture Data),
	0x75, 0x08,	  //      Report Size (8),
	0x95, 0x77,	  //      Report Count (119),
	0xB1, 0x02,	  //      Feature (Variable),
	0x85, 0x05,	  //      Report ID (5),
	0x06, 0x00, 0xFF, //      Usage Page (FF00h),
	0x09, 0xC8,	  //      Usage (C8h),
	0x75, 0x08,	  //      Report Size (8),
	0x95, 0x01,	  //      Report Count (1),
	0xB1, 0x02,	  //      Feature (Variable),
	0x85, 0x09,	  //      Report ID (9),
	0x09, 0xC9,	  //      Usage (C9h),
	0x75, 0x08,	  //      Report Size (8),
	0x95, 0x3F,	  //      Report Count (63),
	0xB1, 0x02,	  //      Feature (Variable),
	0xC0,		  //  End Collection,
};

int ipts_desc_get_size(u8 report);
u8 ipts_desc_get_report(int size);

#endif /* _IPTS_DESC_H_ */

/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTS_HID_H_
#define _IPTS_HID_H_

#include <linux/types.h>

#include "context.h"

#define IPTS_DATA_TYPE_PAYLOAD	    0x0
#define IPTS_DATA_TYPE_ERROR	    0x1
#define IPTS_DATA_TYPE_VENDOR_DATA  0x2
#define IPTS_DATA_TYPE_HID_REPORT   0x3
#define IPTS_DATA_TYPE_GET_FEATURES 0x4

struct ipts_data {
	u32 type;
	u32 size;
	u32 buffer;
	u8 reserved[52];
	u8 data[];
} __packed;

int ipts_hid_input_data(struct ipts_context *ipts, int buffer);
int ipts_hid_init(struct ipts_context *ipts);
void ipts_hid_free(struct ipts_context *ipts);

#endif /* _IPTS_HID_H_ */

/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTS_PROTOCOL_TOUCH_H_
#define _IPTS_PROTOCOL_TOUCH_H_

#include <linux/types.h>

struct ipts_touch_data {
	enum ipts_touch_data_type type;
	u32 size;
	u32 buffer;
	u8 reserved[52];
	u8 data[];
};
static_assert(sizeof(struct ipts_touch_data) == 64);

struct ipts_feedback {
	enum ipts_feedback_type type;
	u32 size;
	u32 transaction;
	u8 reserved[52];
	u8 data[];
};
static_assert(sizeof(struct ipts_feedback) == 64);

struct ipts_stylus_report {
	__be16 timestamp;
	__be16 mode;
	__be16 x;
	__be16 y;
	__be16 pressure;
	__be16 altitude;
	__be16 azimuth;
	__be16 reserved;
};
static_assert(sizeof(struct ipts_stylus_report) == 16);

#endif /* _IPTS_PROTOCOL_TOUCH_H_ */

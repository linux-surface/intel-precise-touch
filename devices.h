/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTS_DEVICES_H_
#define _IPTS_DEVICES_H_

#include <linux/types.h>

/*
 * These names are somewhat random, but without samples from every device
 * it is hard to pinpoint some specific pattern. In fact it could be that
 * every device has it's own version of the stylus protocol, for what we know.
 *
 * IPTS_STYLUS_PROTOCOL_GEN1 can be found on the Surface Book 1, while
 * IPTS_STYLUS_PROTOCOL_GEN2 can be found on the Surface Book 2.
 */
enum ipts_stylus_protocol {
	IPTS_STYLUS_PROTOCOL_GEN1,
	IPTS_STYLUS_PROTOCOL_GEN2
};

struct ipts_device_config {
	u32 vendor_id;
	u32 device_id;
	u32 max_stylus_pressure;
	enum ipts_stylus_protocol stylus_protocol;
};

struct ipts_device_config ipts_devices_get_config(u32 vendor, u32 device);

#endif /* _IPTS_DEVICES_H_ */

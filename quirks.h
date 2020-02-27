/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTS_QUIRKS_H_
#define _IPTS_QUIRKS_H_

#include <linux/types.h>

#include "context.h"

/*
 * Devices using NTRIG digitizers only support 1024 levels of pressure,
 * and use a different format for sending the stylus input data.
 */
#define IPTS_QUIRKS_NTRIG_DIGITIZER BIT(0)

struct ipts_quirks_config {
	u32 vendor_id;
	u32 device_id;
	u32 quirks;
};

u32 ipts_quirks_query(struct ipts_context *ipts);

#endif /* _IPTS_DEVICES_H_ */

// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/types.h>

#include "context.h"
#include "quirks.h"

static const struct ipts_quirks_config ipts_quirks[] = {
	{
		.vendor_id = 0x1B96,
		.device_id = 0x006A,
		.quirks = IPTS_QUIRKS_NTRIG_DIGITIZER,
	},
	{
		.vendor_id = 0x1B96,
		.device_id = 0x005e,
		.quirks = IPTS_QUIRKS_NTRIG_DIGITIZER,
	},
};

u32 ipts_quirks_query(struct ipts_context *ipts)
{
	int i;
	struct ipts_quirks_config cfg;

	for (i = 0; i < ARRAY_SIZE(ipts_quirks); i++) {
		cfg = ipts_quirks[i];

		if (cfg.vendor_id != ipts->device_info.vendor_id)
			continue;
		if (cfg.device_id != ipts->device_info.device_id)
			continue;

		return cfg.quirks;
	}

	// No device was found, so return a default config
	cfg.vendor_id = ipts->device_info.vendor_id;
	cfg.device_id = ipts->device_info.device_id;
	cfg.quirks = 0;

	return cfg.quirks;
}

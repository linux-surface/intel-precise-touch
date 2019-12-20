/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTS_PROTOCOL_COMMANDS_H_
#define _IPTS_PROTOCOL_COMMANDS_H_

#include <linux/build_bug.h>
#include <linux/types.h>

struct ipts_set_mode_cmd {
	enum ipts_sensor_mode sensor_mode;
	u8 reserved[12];
};
static_assert(sizeof(struct ipts_set_mode_cmd) == 16);

/*
 * Commands are sent from the host to the ME
 */
union ipts_command_data {
	struct ipts_set_mode_cmd set_mode;
	u8 reserved[320];
};
struct ipts_command {
	u32 code;
	union ipts_command_data data;
};
static_assert(sizeof(struct ipts_command) == 324);

#endif /* _IPTS_PROTOCOL_COMMANDS_H_ */

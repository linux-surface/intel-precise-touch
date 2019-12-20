/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTS_PROTOCOL_RESPONSES_H_
#define _IPTS_PROTOCOL_RESPONSES_H_

#include <linux/build_bug.h>
#include <linux/types.h>

#include "enums.h"

/*
 * Responses are sent from the ME to the host, reacting to a command.
 */
union ipts_response_data {
	u8 reserved[80];
};
struct ipts_response {
	u32 code;
	enum ipts_me_status status;
	union ipts_response_data data;
};
static_assert(sizeof(struct ipts_response) == 88);

#endif /* _IPTS_PROTOCOL_RESPONSES_H_ */

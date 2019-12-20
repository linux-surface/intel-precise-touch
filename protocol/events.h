/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef _IPTS_PROTOCOL_EVENTS_H_
#define _IPTS_PROTOCOL_EVENTS_H_

/*
 * Helpers to avoid writing boilerplate code.
 * The response to a command code is always 0x8000000x, where x
 * is the command code itself. Instead of writing two definitions,
 * we use macros to calculate the value on the fly instead.
 */
#define IPTS_CMD(COMMAND) IPTS_EVT_##COMMAND
#define IPTS_RSP(COMMAND) (IPTS_CMD(COMMAND) + 0x80000000)

/*
 * Events that can be sent to / received from the ME
 */
#define IPTS_EVT_GET_DEVICE_INFO   0x01
#define IPTS_EVT_SET_MODE          0x02
#define IPTS_EVT_SET_MEM_WINDOW    0x03
#define IPTS_EVT_QUIESCE_IO        0x04
#define IPTS_EVT_SINGLETOUCH_READY 0x05
#define IPTS_EVT_FEEDBACK_READY    0x06
#define IPTS_EVT_CLEAR_MEM_WINDOW  0x07
#define IPTS_EVT_NOTIFY_DEV_READY  0x08

#endif /* _IPTS_PROTOCOL_EVENTS_H_ */

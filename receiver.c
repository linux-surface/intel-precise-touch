// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/types.h>

#include "context.h"
#include "protocol/responses.h"

static void ipts_receiver_handle_response(struct ipts_context *ipts,
		struct ipts_response *msg, u32 msg_len)
{
	// TODO: Handle incoming requests
}

int ipts_receiver_loop(void *data)
{
	u32 msg_len;
	struct ipts_context *ipts;
	struct ipts_response msg;

	ipts = (struct ipts_context *)data;
	dev_dbg(ipts->dev, "Starting receive loop\n");

	while (!kthread_should_stop()) {
		msg_len = mei_cldev_recv(ipts->client_dev,
			(u8 *)&msg, sizeof(msg));

		if (msg_len <= 0) {
			dev_err(ipts->dev, "Error in reading ME message\n");
			continue;
		}

		ipts_receiver_handle_response(ipts, &msg, msg_len);
	}

	dev_dbg(ipts->dev, "Stopping receive loop\n");
	return 0;
}

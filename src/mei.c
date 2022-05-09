// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020-2022 Dorian Stoll
 *
 * Linux driver for Intel Precise Touch & Stylus
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/mei_cl_bus.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "context.h"
#include "control.h"
#include "doorbell.h"
#include "protocol.h"
#include "receiver.h"

static int ipts_mei_set_dma_mask(struct mei_cl_device *cldev)
{
	int ret;

	ret = dma_coerce_mask_and_coherent(&cldev->dev, DMA_BIT_MASK(64));
	if (!ret)
		return 0;

	return dma_coerce_mask_and_coherent(&cldev->dev, DMA_BIT_MASK(32));
}

static int ipts_mei_probe(struct mei_cl_device *cldev,
			  const struct mei_cl_device_id *id)
{
	int ret;
	struct ipts_context *ipts;

	if (ipts_mei_set_dma_mask(cldev)) {
		dev_err(&cldev->dev, "Failed to set DMA mask for IPTS\n");
		return -EFAULT;
	}

	ret = mei_cldev_enable(cldev);
	if (ret) {
		dev_err(&cldev->dev, "Failed to enable MEI device: %d\n", ret);
		return ret;
	}

	ipts = devm_kzalloc(&cldev->dev, sizeof(*ipts), GFP_KERNEL);
	if (!ipts) {
		mei_cldev_disable(cldev);
		return -ENOMEM;
	}

	ipts->cldev = cldev;
	ipts->dev = &cldev->dev;

	// Init with default params
	ipts->mode = IPTS_MODE_SINGLETOUCH;
	ipts->status = IPTS_HOST_STATUS_STOPPED;

	mei_cldev_set_drvdata(cldev, ipts);
	mei_cldev_register_rx_cb(cldev, ipts_receiver_callback);

	ipts->doorbell_loop =
		kthread_run(ipts_doorbell_loop, ipts, "ipts_db_loop");

	return ipts_control_start(ipts);
}

static void ipts_mei_remove(struct mei_cl_device *cldev)
{
	int i;
	struct ipts_context *ipts = mei_cldev_get_drvdata(cldev);

	// Start the shutdown procedure
	ipts_control_stop(ipts);

	// The host needs to flush all buffers and reset the memory window
	// of the hardware to prevent issues during or after suspend.
	// Wait until all of that is done, but not for more than 500ms.
	for (i = 0; i < 20; i++) {
		if (ipts->status == IPTS_HOST_STATUS_STOPPED)
			break;

		msleep(25);
	}

	kthread_stop(ipts->doorbell_loop);
	mei_cldev_disable(cldev);
}

static struct mei_cl_device_id ipts_mei_device_id_table[] = {
	{ .uuid = IPTS_MEI_UUID, .version = MEI_CL_VERSION_ANY },
	{},
};
MODULE_DEVICE_TABLE(mei, ipts_mei_device_id_table);

static struct mei_cl_driver ipts_mei_driver = {
	.id_table = ipts_mei_device_id_table,
	.name = "ipts",
	.probe = ipts_mei_probe,
	.remove = ipts_mei_remove,
};
module_mei_cl_driver(ipts_mei_driver);

MODULE_DESCRIPTION("IPTS touchscreen driver");
MODULE_AUTHOR("Dorian Stoll <dorian.stoll@tmsp.io>");
MODULE_LICENSE("GPL");

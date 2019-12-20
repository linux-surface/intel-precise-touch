# SPDX-License-Identifier: GPL-2.0-or-later
#
# Makefile for the IPTS touchscreen driver
#

MODULE_NAME    := "ipts"
MODULE_VERSION := "2019-12-20"

obj-$(CONFIG_TOUCHSCREEN_IPTS) += ipts.o
ipts-objs := control.o
ipts-objs += hid.o
ipts-objs += init.o
ipts-objs += receiver.o

sources := Makefile
sources += dkms.conf
sources += context.h
sources += control.c
sources += control.h
sources += hid.c
sources += hid.h
sources += init.c
sources += protocol/commands.h
sources += protocol/enums.h
sources += protocol/events.h
sources += protocol/responses.h
sources += receiver.c
sources += receiver.h

KVERSION := "$(shell uname -r)"
KDIR := /lib/modules/$(KVERSION)/build
MDIR := /usr/src/$(MODULE_NAME)-$(MODULE_VERSION)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) CONFIG_TOUCHSCREEN_IPTS=m modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) CONFIG_TOUCHSCREEN_IPTS=m clean

dkms-install: $(sources)
	mkdir -p $(MDIR)
	cp -t $(MDIR) $(sources)
	dkms add $(MODULE_NAME)/$(MODULE_VERSION)
	dkms build $(MODULE_NAME)/$(MODULE_VERSION)
	dkms install $(MODULE_NAME)/$(MODULE_VERSION)

dkms-uninstall:
	modprobe -r $(MODULE_NAME) || true
	dkms uninstall $(MODULE_NAME)/$(MODULE_VERSION) || true
	dkms remove $(MODULE_NAME)/$(MODULE_VERSION) || true
	rm -rf $(MDIR)

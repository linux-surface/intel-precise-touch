# SPDX-License-Identifier: GPL-2.0-or-later
#
# Makefile for the IPTS touchscreen driver
#

MODULE_NAME    := "ipts"
MODULE_VERSION := "2019-12-20"

obj-$(CONFIG_TOUCHSCREEN_IPTS) += ipts.o
ipts-objs := control.o
ipts-objs += devices.o
ipts-objs += fpmath.o
ipts-objs += hid.o
ipts-objs += init.o
ipts-objs += params.o
ipts-objs += receiver.o
ipts-objs += resources.o
ipts-objs += singletouch.o
ipts-objs += stylus.o

sources := Makefile
sources += Kconfig
sources += dkms.conf
sources += context.h
sources += control.c
sources += control.h
sources += devices.c
sources += devices.h
sources += fpmath.c
sources += fpmath.h
sources += hid.c
sources += hid.h
sources += init.c
sources += params.c
sources += params.h
sources += protocol/commands.h
sources += protocol/enums.h
sources += protocol/events.h
sources += protocol/responses.h
sources += protocol/touch.h
sources += receiver.c
sources += receiver.h
sources += resources.c
sources += resources.h
sources += singletouch.c
sources += singletouch.h
sources += stylus.c
sources += stylus.h

KVERSION := "$(shell uname -r)"
KDIR := /lib/modules/$(KVERSION)/build
MDIR := /usr/src/$(MODULE_NAME)-$(MODULE_VERSION)

CFLAGS_fpmath.o := -mhard-float -msse2

all:
	$(MAKE) -C $(KDIR) M=$(PWD) CONFIG_TOUCHSCREEN_IPTS=m modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) CONFIG_TOUCHSCREEN_IPTS=m clean

check:
	$(KDIR)/scripts/checkpatch.pl -f -q --no-tree $(sources)

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

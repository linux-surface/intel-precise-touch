# SPDX-License-Identifier: GPL-2.0-or-later
#
# Makefile for the IPTS touchscreen driver
#

obj-$(CONFIG_MISC_IPTS) += ipts.o
ipts-objs := src/cmd.o
ipts-objs += src/control.o
ipts-objs += src/desc.o
ipts-objs += src/doorbell.o
ipts-objs += src/hid.o
ipts-objs += src/mei.o
ipts-objs += src/receiver.o
ipts-objs += src/resources.o

MODULE_NAME    := ipts
MODULE_VERSION := 2022-05-08

sources := Makefile
sources += Kconfig
sources += dkms.conf
sources += src/cmd.c
sources += src/cmd.h
sources += src/context.h
sources += src/control.c
sources += src/control.h
sources += src/desc.c
sources += src/desc.h
sources += src/doorbell.c
sources += src/doorbell.h
sources += src/hid.c
sources += src/hid.h
sources += src/mei.c
sources += src/protocol.h
sources += src/receiver.c
sources += src/receiver.h
sources += src/resources.c
sources += src/resources.h

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
MDIR := /usr/src/$(MODULE_NAME)-$(MODULE_VERSION)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) CONFIG_MISC_IPTS=m modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) CONFIG_MISC_IPTS=m clean

check:
	$(KDIR)/scripts/checkpatch.pl -f -q --no-tree --ignore EMBEDDED_FILENAME $(sources)

check_strict:
	$(KDIR)/scripts/checkpatch.pl -f -q --no-tree --strict --ignore EMBEDDED_FILENAME $(sources)

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

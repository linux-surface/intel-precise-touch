# Intel Precise Touch & Stylus

Linux kernel driver for Intels IPTS touchscreen technology. With IPTS, the
Intel Management Engine acts as an interface for a touch controller, that
returns either raw capacitive touch data (for multitouch mode), or HID report
data (for singletouch mode). This data is processed outside of the ME 
and then relayed into the HID subsystem of the operating system.

### Original IPTS driver
The original driver for IPTS was released by Intel in late 2016
(https://github.com/ipts-linux-org/ipts-linux-new). It uses GuC submission
to process raw touch input data on the GPU, using OpenCL firmware extracted
from Windows.

An updated version of the driver can be found here:
https://github.com/linux-surface/kernel/tree/v4.19-surface-devel/drivers/misc/ipts

### This driver
This driver is loosely based on the original implementation. The main difference
is, that due to changes in the i915 graphics driver we cannot easily use GuC
submission anymore on kernel 5.4+.

This driver is not using GuC submission, instead it forwards the raw touch
data to userspace where it gets parsed. The reason for moving the parsing
to userspace is that it heavily relies on floating point mathematic, which
is not available in the kernel directly.

Other changes are a significant removal of unused and dead code compared to
the driver provided by Intel.

### Userspace daemon
The userspace daemon that parses the data and handles input devices can be
found here: https://github.com/linux-surface/iptsd

### Working
* Stylus input (gen4-6)
* Singletouch finger input (gen7)

### Not working
* Multitouch finger input
* Stylus input on gen7

### Building (in-tree)
* Apply the patches from `patches/` to your kernel
* Add the files in this repository to `drivers/misc/ipts`
* Add `source "drivers/misc/ipts/Kconfig"` to
  `drivers/misc/Kconfig`
* Add `obj-y += ipts/` to `drivers/misc/Makefile`
* Enable the driver by setting `CONFIG_MISC_IPTS=m` in the kernel config

### Building (out-of-tree)
* Clone this repository and `cd` into it
* Make sure you applied the patches from `patches/` to your kernel
* Run `make all`
* Run `sudo insmod ipts.ko`

### Building (DKMS)
* Clone this repository and `cd` into it
* Make sure you applied the patches from `patches/` to your kernel
* Run `sudo make dkms-install`
* Reboot

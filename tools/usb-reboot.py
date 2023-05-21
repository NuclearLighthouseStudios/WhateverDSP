#!/usr/bin/env python

import sys
import errno
import usb.core
import usb.util

dev = usb.core.find(idVendor=0xdead, idProduct=0xc0de)

if dev is None:
	print("Device not found!", file=sys.stderr)
	exit(1)

try:
	dev.ctrl_transfer(0x40, 0x01)
except usb.core.USBError as e:
	# Since the device just kinda yeets itself from the bus on reset
	# a successful reset results in a broken pipe error.
	if e.errno == errno.EPIPE:
		print("Device reset successfully")
	else:
		print(e)

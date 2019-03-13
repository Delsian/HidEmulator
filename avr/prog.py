#!/usr/bin/env python

'''
Created on Dec 13, 2018

@author: Eugene.Krashtan@opensynergy.com

'''

import argparse
import kmod
import subprocess

MODNAME = u'i2c_bcm2835'
PROG_CONF = 'hids_gpio.conf'

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument("port", help="HID port number",type=int)
	parser.add_argument("addr", help="i2c slave address",type=int)
	args = parser.parse_args()
	print args

	k = kmod.Kmod()
	for m in k.loaded():
		if (m.name == MODNAME):
			print 'i2c stop'
			k.rmmod(m.name)
			break

	subprocess.call(["make","clean"])
	retcode = subprocess.call(["make",'CFLAGS=-DTWI_SLAVE_ADDR='+str(args.addr)])
	if (retcode==0):
		subprocess.call(["avrdude","-C" + PROG_CONF, 
			"-pattiny85",
			"-cpi_hid" + str(args.port),
			"-v",
			"-u",
			"-Uflash:w:main.hex:i"])

	k.modprobe(MODNAME)

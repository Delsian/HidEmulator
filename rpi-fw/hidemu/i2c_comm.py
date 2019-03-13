'''
Created on Dec 13, 2018

@author: Eugene.Krashtan@opensynergy.com
'''

import smbus

HEADER_MAGIC = 0x5a

class I2C(object):

    def __init__(self,bus):
        self.i2c = smbus.SMBus(bus)
       
    def send_coord(self, addr, absx, absy, buttons):
        lsbx = int(absx) & 0xff
        msbx = (int(absx) >> 8) & 0xf
        msby = (int(absy)>>4) & 0xff
        msbx += (int(absy) << 4) & 0xf0
        values = [lsbx, msbx, msby, buttons]
        print (values)
        self.i2c.write_i2c_block_data(addr, HEADER_MAGIC, values)
        
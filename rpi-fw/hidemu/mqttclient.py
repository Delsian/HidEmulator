'''
Created on Dec 13, 2018

@author: Eugene.Krashtan@opensynergy.com
'''
import paho.mqtt.client as mqtt
from i2c_comm import I2C

CLIENT_NAME =  'opsy'
I2C_BUS_NUMBER = 1

class MqttClient(object):

    def __init__(self):
        self.connected = False
        self.topics = {}
        self.i2c = I2C(I2C_BUS_NUMBER)
    
    def connect(self, broker_addr, broker_port = 1883, login = None, passwd = None):
        if (len(self.topics) == 0):
            print("Topic list is empty, exiting...")
            exit(1)
            
        self.client = mqtt.Client(CLIENT_NAME)
        if (login):
            self.client.username_pw_set(login,passwd)
        self.client.connect(broker_addr,broker_port)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
    
    def on_connect(self, client, userdata, flags, rc):
        for topic in self.topics:
            print("Subscribe "+ topic)
            self.client.subscribe(topic)
        print("Connected with result code "+str(rc))
        
    def on_message(self, client, userdata, msg):
        args = msg.payload.decode().split(",")
        if (len(args) == 3):
            print("Click at " + args[0] + ":" + args[1] + ", b" + args[2] + " to ch 0x%x"%self.topics[msg.topic])
            self.i2c.send_coord(self.topics[msg.topic], int(args[0]), int(args[1]) , int(args[2]))
        else:
            print("Wrong message format: "+ msg.payload)
        
    def set_topic(self, topic, addr):
        self.topics[topic] = addr
        
    def loop(self):
        self.client.loop_forever()
        
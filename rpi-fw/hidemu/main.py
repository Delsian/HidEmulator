#!/usr/bin/env python
'''
Created on Dec 13, 2018

@author: Eugene.Krashtan@opensynergy.com

'''

from mqttclient import MqttClient
import json

# ToDo: move to commandline
MY_NAME = 'rpi1'

if __name__ == '__main__':
    with open("config.json", "r") as read_file:
        confdata = json.load(read_file)
    
    mqttconf = confdata["MQTT"]
    hidconf = confdata["HID"]
    mqtt = MqttClient()
    for hiddev in hidconf:
        if (hiddev['device'] == MY_NAME):
            mqtt.set_topic(hiddev['topic'], hiddev['port'])
    if 'login' in mqttconf.keys():     
        mqtt.connect(mqttconf['addr'],
                     mqttconf['port'],
                     mqttconf['login'],
                     mqttconf['password'])
    else:
        mqtt.connect(mqttconf['addr'], mqttconf['port'])
    mqtt.loop()
    
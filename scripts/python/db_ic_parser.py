# -*- encoding: utf-8 -*-


# Sapronov Alexander
# sapronov.alexander92@gmail.com
# 2014


"""
reqs:
ply
CppHeaderParser
"""


import re

from CppHeaderParser import CppHeader


def getValues(l, n=4):
    r = r'\(%s\)' % (','.join(['(.*)' for x in xrange(0, n)]))
    items = [x.strip().translate(None, ' "?.!/;')
             for x in re.search(r, l).groups()]
    return items


class Stack(object):

    def __init__(self):
        self.__storage = []

    def isEmpty(self):
        return len(self.__storage) == 0

    def push(self, p):
        self.__storage.append(p)

    def pop(self):
        return self.__storage.pop()


class Monitor(object):

    def __init__(self, name, number):
        self.name = name
        self.number = number


class Device(object):

    def __init__(self, level, name, shortname):
        self.startV = 0
        self.stopV = 0
        self.chV = 0
        self.channels = []
        self.level = level
        self.name = name
        self.shortname = shortname

    def start(self):
        self.startV = 1

    def stop(self):
        self.stopV = 1

    def addch(self, *args):
        self.channels.append(Monitor(args[0], args[1]))
        self.chV += 1


class DeviceInfo(object):

    def __init__(self):
        # read enums
        filepath = 'db_ic.h'

        allData = self.readFile(filepath)

        cppheader = CppHeader(allData, argType='string')
        self.devices = cppheader.enums[0].get('values')

        data = allData.split('};')[2].split('\n')

        r = r"{(.*),(.*),(.*)}"

        for line in data:
            try:
                items = [x.strip() for x in re.search(r, line).groups()]
                self.addPropToDevice(
                    items[0].strip(),
                    'phys_max',
                    items[1].strip())
                self.addPropToDevice(
                    items[0].strip(),
                    'phys_min',
                    items[2].strip())
            except AttributeError:
                pass

        treeData = allData.split('};')[1]
        self.parseTree(treeData)

    def getDeviceInfo(self, name):
        for device in self.devices:
            if device.get('name', '') == name:
                return device
        return {}

    def valueOfDevice(self, name):
        return self.getDeviceInfo(name).get('value', '-nan')

    def addPropToDevice(self, name, propname, propvalue):
        self.getDeviceInfo(name)[propname] = propvalue

    def readFile(self, filepath, lines=False):
        data = ''
        with open(filepath, 'r') as myfile:
            if lines:
                data = myfile.readlines()
            else:
                data += myfile.read()
        return data

    def parseTree(self, treeData):

        TAG_START_GLOBAL = 'GLOBTABBER_START'
        TAG_STOP_GLOBAL = 'GLOBELEM_END'
        TAG_START_SUBELEM = 'SUBELEM_START'
        TAG_CHANNEL_MAIBOX = 'MAILBOX'
        TAG_STOP_SUBELEM = 'SUBELEM_END'
        TAG_START_TAB = 'TABBER_START'
        TAG_STOP_TAB = 'SUBELEM_END'

        cntProp = {
            TAG_START_GLOBAL: 2,
            TAG_STOP_GLOBAL: 2,
            TAG_START_SUBELEM: 4,
            TAG_CHANNEL_MAIBOX: 2,
            TAG_STOP_SUBELEM: 2,
            TAG_START_TAB: 3,
            TAG_STOP_TAB: 2,
        }

        devices = {}

        cur_level = ''
        cur_device = ''
        lines = treeData.split('\n')
        stack = Stack()

        for line in lines:
            l = line.strip()

            for key, value in cntProp.items():
                if l.startswith(key):
                    items = getValues(l, cntProp[key])

                    if key == TAG_START_SUBELEM or key == TAG_START_TAB:
                        name_channel = items[1]
                        shortname = items[0]
                        cur_level += '.%s' % shortname
                        # print cur_level
                        dev = cur_device = Device(
                            level=cur_level,
                            name=name_channel,
                            shortname=shortname)
                        dev.start()
                        devices[name_channel] = dev
                        stack.push(dev)

                    if key == TAG_START_GLOBAL:
                        cur_level += items[0]

                    if key == TAG_CHANNEL_MAIBOX:
                        channel_info = [items[0], self.valueOfDevice(items[1])]
                        cur_device.addch(*channel_info)

                    if key == TAG_STOP_SUBELEM:
                        stack.pop().stop()
                        cur_level = '.'.join(cur_level.split('.')[:-1])

        for device in devices.values():
            print device.level


def start():
    a = DeviceInfo()

if __name__ == '__main__':
    start()

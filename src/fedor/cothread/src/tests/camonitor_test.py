#!/usr/bin/env python2.6

'''camonitor minimal example'''

from __future__ import print_function

from cothread import WaitForQuit
from cothread.catools import *


def callback(value):
    '''monitor callback'''
    print(value.name, value)


camonitor('TS-DI-EBPM-01:FR:WFX', callback)

WaitForQuit()

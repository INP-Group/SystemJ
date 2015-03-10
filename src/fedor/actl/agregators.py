# -*- encoding: utf-8 -*-
import numpy as np

from actl.cxchan import *
from cothread.catools import *


# sum of channel values calculator
# it's read-only
class middleSummer(object):
    def __init__(self, srcnames, midname):
        self.midchan = cxchan(midname)

        self.srcchans = []
        for x in range(len(srcnames)):
            self.srcchans.append(cxchan(srcnames[x]))
            self.srcchans[x].valueChanged.connect(self.src_cb)

        self.vals = np.zeros(len(srcnames))
        self.aval = np.zeros(1)

    def src_cb(self, chan, value):
        ind = self.srcchans.index(chan)
        self.aval = self.aval - self.vals[ind] + value
        self.vals[ind] = value
        self.midchan.setValue(self.aval)


class maskAgregatorEpics:
    def __init__(self, pv_namelist, change_cb=None):
        self.pvs = pv_namelist
        self.agregatedVal = 0
        self.change_cb = change_cb
        self.vals = np.zeros(len(pv_namelist), dtype=np.uint8)
        self.pvs = camonitor(pv_namelist, self.cb)

    def cb(self, value, index):
        self.vals[index] = value
        new_agregatedVal = np.any(self.vals)
        if self.agregatedVal != new_agregatedVal:
            self.agregatedVal = new_agregatedVal
            if self.change_cb is not None:
                self.change_cb()

    # BIG Warning!!!
    def setValue(self, value):
        caput(self.pvs, value, repeat_value=True)


class maskAgregatorEpics2cx:
    def __init__(self, pv_namelist, cxname):
        self.pvs = pv_namelist
        self.cxname = cxname
        self.chan = cxchan(self.cxname)
        self.agregatedVal = 0
        self.vals = np.zeros(len(pv_namelist), dtype=np.uint8)
        self.pvs = camonitor(pv_namelist, self.cb)

    def cb(self, value, index):
        self.vals[index] = value
        new_agregatedVal = np.any(self.vals)
        if self.agregatedVal != new_agregatedVal:
            self.agregatedVal = new_agregatedVal
            self.chan.setValue(self.agregatedVal)

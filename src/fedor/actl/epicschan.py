# workaround to avoid epics problems

import math

from PyQt4.QtCore import *

from cothread.catools import *


class epicschan(QObject):
    valueChanged = pyqtSignal(QObject, float)
    valueMeasured = pyqtSignal(QObject, float)
    processNeeded = pyqtSignal()

    def __init__(self, pvname):
        super(QObject, self).__init__()
        self.pvname = pvname

        self.pv = camonitor(pvname, self.cb)

        self.prev_val = 0.0  # value to compare with just received
        self.val = 0.0
        self.tolerance = 0.0

    def cb(self, val):
        self.val = val


    def process(self):
        if math.fabs(self.val - self.prev_val) > self.tolerance or self.first_cycle:
            self.first_cycle = False
            self.prev_val = self.val
            self.valueChanged.emit(self, self.val)
        self.valueMeasured.emit(self, self.val)


    def setValue(self, value):
        self.cdr.SetSimpleChanVal(self.cx_chan, value)

    def setTolerance(self, tolerance):
        self.tolerance = tolerance

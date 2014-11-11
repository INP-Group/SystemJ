# cdr channel class by Fedor Emanov

import math

from PyQt4.QtCore import *


class cxchan(QObject):
    valueChanged = pyqtSignal(QObject, float)
    valueMeasured = pyqtSignal(QObject, float)

    def __init__(self, name):
        super(QObject, self).__init__()
        self.name = name
        self.cdr = cdr()
        self.cx_chan = self.cdr.RegisterSimpleChan(name, self.cb)

        self.prev_val = 0.0  # value to compare with just received
        self.val = 0.0
        self.tolerance = 0.0
        self.first_cycle = True

    # the callback for cdr
    def cb(self, handle, val, params):
        self.val = val
        if math.fabs(self.val - self.prev_val) > self.tolerance or self.first_cycle:
            self.first_cycle = False
            self.prev_val = self.val
            self.valueChanged.emit(self, self.val)
        self.valueMeasured.emit(self, self.val)

        return 0

    def setValue(self, value):
        self.cdr.SetSimpleChanVal(self.cx_chan, value)

    def setTolerance(self, tolerance):
        self.tolerance = tolerance


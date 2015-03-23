# -*- encoding: utf-8 -*-


import math

from basechannel import BaseChannel
from cdr_wrapper import Cdr
from PyQt4.QtCore import *
from settings import CDR_LIB_PATH


class BaseDataChannel(BaseChannel):
    valueChanged = pyqtSignal(QObject, float)
    valueMeasured = pyqtSignal(QObject, float)

    def __init__(self, name, personal_name=None):
        QObject.__init__(self)
        if personal_name is None:
            self.personal_name = name
        else:
            self.personal_name = personal_name
        # todo hardcode
        self.name = name
        self.cdr = Cdr(CDR_LIB_PATH)
        # self.cdr = Cdr()
        self.cx_chan = self.cdr.RegisterSimpleChan(name, self.callback)

        self.prev_val = 0.0  # value to compare with just received
        self.val = 0.0
        self.tolerance = 0.0
        self.first_cycle = True

    def getPName(self):
        return self.personal_name

    def processing(self, *args):
        raise NotImplemented("Callback")

    # the callback for cdr
    def callback(self, handle, val, params):
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

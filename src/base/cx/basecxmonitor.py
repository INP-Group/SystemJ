# -*- encoding: utf-8 -*-
import math

from project.settings import CDR_LIB_PATH
from PyQt4.QtCore import *
from src.base.basedatamonitor import BaseDataMonitor
from src.base.cx.cdr_wrapper import Cdr


class BaseCXMonitor(BaseDataMonitor):

    def __init__(self, name, personal_name=None):
        super(BaseCXMonitor, self).__init__(name, personal_name)
        self.cdr = Cdr(CDR_LIB_PATH)
        self.cx_chan = self.cdr.RegisterSimpleChan(self.name, self.callback)

    # the callback for cdr
    def callback(self, handle, val, params):
        self.val = val
        if math.fabs(
                self.val - self.prev_val) > self.tolerance or self.first_cycle:
            self.first_cycle = False
            self.prev_val = self.val
            self.valueChanged.emit(self, self.val)
        self.valueMeasured.emit(self, self.val)

        return 0

    def setValue(self, value):
        self.cdr.SetSimpleChanVal(self.cx_chan, value)

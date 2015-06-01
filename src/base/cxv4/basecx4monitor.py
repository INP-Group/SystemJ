# -*- encoding: utf-8 -*-
import math

from PyQt4.QtCore import *
from src.base.basedatamonitor import BaseDataMonitor
import ccxv4.ccda as ccda
# imports for testing

class BaseCX4Monitor(BaseDataMonitor):
    def __init__(self, name, personal_name=None):
        super(BaseCX4Monitor, self).__init__(name, personal_name)
        assert int(self.name)
        self.cx_chan = ccda.sdchan(None, "", "cx::mid:60.NAME.%s" % self.name)
        self.cx_chan.valueChanged.connect(self.callback)

    # the callback for cdr
    def callback(self, channel):
        self.val = channel.val
        if math.fabs(
                        self.val - self.prev_val) > self.tolerance or self.first_cycle:
            self.first_cycle = False
            self.prev_val = self.val
            self.valueChanged.emit(self, self.val)
        self.valueMeasured.emit(self, self.val)

        return 0

    def setValue(self, value):
        self.cx_chan.setValue(value)

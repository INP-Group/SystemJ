# -*- encoding: utf-8 -*-

from basemonitor import BaseMonitor
from PyQt4.QtCore import *


class BaseDataMonitor(BaseMonitor):
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
        self.prev_val = 0.0  # value to compare with just received
        self.val = 0.0
        self.tolerance = 0.0
        self.first_cycle = True

    def getPName(self):
        return self.personal_name

    def processing(self, *args):
        raise NotImplemented('Callback')

    def setTolerance(self, tolerance):
        self.tolerance = tolerance

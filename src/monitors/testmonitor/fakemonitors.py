# -*- encoding: utf-8 -*-

import datetime

from PyQt4.QtCore import QObject, pyqtSignal
from src.base.monitor import TestMonitor


class CustomFakeMonitor(TestMonitor):

    valueToStorage = pyqtSignal(QObject, object)

    def __init__(self, name, frequency=100):
        super(CustomFakeMonitor, self).__init__(name)
        self.startTimer(frequency)

    def processing(self, *args):
        now_time = str(datetime.datetime.now())
        handle, val, params = self._gfa(args, 1), self._gfa(args,
                                                            2), self._gfa(args,
                                                                          3)

        text = '(%s), %s %s %s' % (self.personal_name, handle, val, params)

        self.default_log(text)
        self.send_data(self.default_form([self.name, self.personal_name,
                                          handle, now_time]))
        return 0


class EasyFakeMonitor(CustomFakeMonitor):

    valueToStorage = pyqtSignal(QObject, object)

    def __init__(self, name):
        super(EasyFakeMonitor, self).__init__(name, 100)


class HardFakeMonitor(CustomFakeMonitor):

    valueToStorage = pyqtSignal(QObject, object)

    def __init__(self, name):
        super(HardFakeMonitor, self).__init__(name, 10)


class MiddleFakeMonitor(CustomFakeMonitor):

    valueToStorage = pyqtSignal(QObject, object)

    def __init__(self, name):
        super(MiddleFakeMonitor, self).__init__(name, 50)

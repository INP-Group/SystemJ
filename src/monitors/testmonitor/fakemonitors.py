# -*- encoding: utf-8 -*-


import datetime

from src.base.monitor import TestMonitor


class CustomFakeMonitor(TestMonitor):

    def __init__(self, name, frequency=100):
        super(CustomFakeMonitor, self).__init__(name)
        self.startTimer(frequency)
        
    def processing(self, *args):
        now_time = str(datetime.datetime.now())
        handle, val, params = self._gfa(
            args, 1), self._gfa(
            args, 2), self._gfa(
            args, 3)

        text = '(%s), %s %s %s' % (self.personal_name, handle, val, params)

        self.default_log(text)
        self.send_message(
            self.default_form([self.name, self.personal_name, handle, now_time]))
        return 0


class EasyFakeMonitor(CustomFakeMonitor):
    def __init__(self, name):
        super(EasyFakeMonitor, self).__init__(name, 100)


class HardFakeMonitor(CustomFakeMonitor):
    def __init__(self, name):
        super(HardFakeMonitor, self).__init__(name, 10)

class MiddleFakeMonitor(CustomFakeMonitor):
    def __init__(self, name):
        super(MiddleFakeMonitor, self).__init__(name, 50)

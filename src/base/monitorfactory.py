# -*- encoding: utf-8 -*-

from src.monitors.cxmonitors.deltamonitor import DeltaCXMonitor
from src.monitors.cxmonitors.ntimemonitor import NTimeCXMonitor
from src.monitors.cxmonitors.scalarmonitor import ScalarCXMonitor
from src.monitors.cxmonitors.simplemonitor import SimpleCXMonitor
from src.monitors.testmonitor.fakemonitors import *


class MonitorFactory(object):

    @staticmethod
    def factory(monitor_type, name, personale_name=None, frequency=100):
        if monitor_type == 'ScalarMonitor':
            return ScalarCXMonitor(name, personale_name)
        if monitor_type == 'NTimeMonitor':
            return NTimeCXMonitor(name, personale_name)
        if monitor_type == 'DeltaMonitor':
            return DeltaCXMonitor(name, personale_name)
        if monitor_type == 'SimpleMonitor':
            return SimpleCXMonitor(name, personale_name)
        if monitor_type == 'HardFakeMonitor':
            return HardFakeMonitor(name)
        if monitor_type == 'MiddleFakeMonitor':
            return MiddleFakeMonitor(name)
        if monitor_type == 'EasyFakeMonitor':
            return EasyFakeMonitor(name)
        if monitor_type == 'CustomFakeMonitor':
            return CustomFakeMonitor(name, frequency=frequency)

        assert 0, 'Bad shape creation: ' + monitor_type

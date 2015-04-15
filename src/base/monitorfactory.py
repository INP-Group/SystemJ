# -*- encoding: utf-8 -*-

from src.monitors.datamonitor.deltamonitor import DeltaCXMonitor
from src.monitors.testmonitor.fakemonitors import *
from src.monitors.datamonitor.ntimemonitor import NTimeCXMonitor
from src.monitors.datamonitor.scalarmonitor import ScalarCXMonitor
from src.monitors.datamonitor.simplemonitor import SimpleCXMonitor


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

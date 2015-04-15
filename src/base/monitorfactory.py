# -*- encoding: utf-8 -*-

from src.monitors.datamonitor.deltamonitor import DeltaCXMonitor
from src.monitors.datamonitor.fakemonitor import FakeMonitor
from src.monitors.datamonitor.ntimemonitor import NTimeCXMonitor
from src.monitors.datamonitor.scalarmonitor import ScalarCXMonitor
from src.monitors.datamonitor.simplemonitor import SimpleCXMonitor


class MonitorFactory(object):

    @staticmethod
    def factory(monitor_type, name, personale_name=None):
        if monitor_type == 'ScalarMonitor':
            return ScalarCXMonitor(name, personale_name)
        if monitor_type == 'NTimeMonitor':
            return NTimeCXMonitor(name, personale_name)
        if monitor_type == 'DeltaMonitor':
            return DeltaCXMonitor(name, personale_name)
        if monitor_type == 'SimpleMonitor':
            return SimpleCXMonitor(name, personale_name)
        if monitor_type == 'FakeMonitor':
            return FakeMonitor(name, personale_name)

        assert 0, 'Bad shape creation: ' + monitor_type

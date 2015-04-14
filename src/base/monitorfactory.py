# -*- encoding: utf-8 -*-


from src.channels.deltamonitor import DeltaCXMonitor
from src.channels.ntimemonitor import NTimeCXMonitor
from src.channels.scalarmonitor import ScalarCXMonitor
from src.channels.simplemonitor import SimpleCXMonitor


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

        assert 0, 'Bad shape creation: ' + monitor_type


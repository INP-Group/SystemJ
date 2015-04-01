# -*- encoding: utf-8 -*-


from src.channels.deltamonitor import DeltaMonitor
from src.channels.ntimemonitor import NTimeMonitor
from src.channels.scalarmonitor import ScalarMonitor
from src.channels.simplemonitor import SimpleMonitor


class MonitorFactory(object):

    def factory(type, name, personale_name=None):
        if type == 'ScalarMonitor':
            return ScalarMonitor(name, personale_name)
        if type == 'NTimeMonitor':
            return NTimeMonitor(name, personale_name)
        if type == 'DeltaMonitor':
            return DeltaMonitor(name, personale_name)
        if type == 'SimpleMonitor':
            return SimpleMonitor(name, personale_name)

        assert 0, 'Bad shape creation: ' + type

    factory = staticmethod(factory)

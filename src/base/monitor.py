# -*- encoding: utf-8 -*-


import datetime
import os

from project.logs import log_info
from project.settings import LOG
from project.settings import LOG_FOLDER
from src.base.basedatamonitor import BaseDataMonitor
from src.base.cx.basecxmonitor import BaseCXMonitor
from src.monitors.servicemonitor.zeromqmonitor import ZeroMQMonitor


class TestMonitor(BaseDataMonitor, ZeroMQMonitor):

    def __init__(self, name, personal_name=None):
        BaseDataMonitor.__init__(self, name, personal_name)
        ZeroMQMonitor.__init__(self)

        self.valueChanged.connect(self.processing)

        self.startTimer(100)
        self._post_init()

    def timerEvent(self, event):
        self.val += 10
        if self.val > 100:
            self.val = 0
        self.valueChanged.emit(self, self.val)
        self.valueMeasured.emit(self, self.val)

    def setValue(self, value):
        self.val = value

    def setTolerance(self, tolerance):
        self.tolerance = tolerance

    def _post_init(self):
        None

    def get_message(self, text):
        return '[%s] [%s]: %s' % (datetime.datetime.now(),
                                  self.__class__.__name__,
                                  text)

    def default_log(self, text):
        with open(os.path.join(LOG_FOLDER, 'default_log.log'), 'a') as f:
            f.write('%s\n' % self.get_message(text))
            if LOG:
                log_info(self.get_message(text))

    def default_form(self, args):
        return '\t'.join(str(v).replace("'", '') for v in args)


class CXMonitor(BaseCXMonitor, ZeroMQMonitor):

    def __init__(self, name, personal_name=None):
        BaseCXMonitor.__init__(self, name, personal_name)
        ZeroMQMonitor.__init__(self)

        self.valueChanged.connect(self.processing)
        self._post_init()

    def _post_init(self):
        pass

    def get_message(self, text):
        return '[%s] [%s]: %s' % (datetime.datetime.now(),
                                  self.__class__.__name__,
                                  text)

    def default_log(self, text):
        with open(os.path.join(LOG_FOLDER, 'default_log.log'), 'a') as f:
            f.write('%s\n' % self.get_message(text))
            if LOG:
                log_info(self.get_message(text))

    def default_form(self, args):
        return '\t'.join(str(v).replace("'", '') for v in args)

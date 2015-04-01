# -*- encoding: utf-8 -*-


import datetime
import os

from basedatamonitor import BaseDataMonitor
from project.settings import LOG_FOLDER
from src.base.zeromqmonitor import ZeroMQMonitor


class Monitor(BaseDataMonitor, ZeroMQMonitor):

    def __init__(self, name, personal_name=None):
        BaseDataMonitor.__init__(self, name, personal_name)
        ZeroMQMonitor.__init__(self)

        self.valueChanged.connect(self.processing)

        self._post_init()

    def _post_init(self):
        None

    def get_message(self, text):
        return '[%s] [%s]: %s' % (datetime.datetime.now(),
                                  self.__class__.__name__,
                                  text)

    def default_log(self, text):
        with open(os.path.join(LOG_FOLDER, 'default_log.log'), 'a') as f:
            f.write('%s\n' % self.get_message(text))
            print self.get_message(text)

    def default_form(self, args):
        return '\t'.join(str(v).replace("'", '') for v in args)

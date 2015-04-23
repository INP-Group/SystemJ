# -*- encoding: utf-8 -*-

import datetime

from PyQt4.QtCore import QObject
from PyQt4.QtCore import pyqtSignal
from src.base.monitor import CXMonitor


class SimpleCXMonitor(CXMonitor):
    valueToStorage = pyqtSignal(QObject, object)

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

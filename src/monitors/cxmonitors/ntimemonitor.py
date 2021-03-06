# -*- encoding: utf-8 -*-

import datetime
import time

from PyQt4.QtCore import QObject, pyqtSignal
from src.base.monitor import CXMonitor


class NTimeCXMonitor(CXMonitor):

    valueToStorage = pyqtSignal(QObject, object)

    def processing(self, *args):
        now = time.time()
        now_time = str(datetime.datetime.now())
        if now - self.start_time > self.get_property('timedelta'):
            self.start_time = now
            handle, val, params = self._gfa(args, 1), \
                self._gfa(args, 2), \
                self._gfa(args, 3)
            text = '(%s), %s %s %s' % (self.personal_name, handle, val, params)

            self.default_log(text)
            self.send_data(self.default_form([self.name, self.personal_name,
                                              handle, now_time]))

        return 0

    def _post_init(self):
        self.start_time = time.time()

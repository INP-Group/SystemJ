# -*- encoding: utf-8 -*-


import datetime
import math
import time

from src.base.monitor import CXMonitor
from PyQt4.QtCore import QObject, pyqtSignal


class ScalarCXMonitor(CXMonitor):

    valueToStorage = pyqtSignal(QObject, object)

    def processing(self, *args):
        now = time.time()
        now_time = str(datetime.datetime.now())
        handle, val, params = self._gfa(args, 1), \
            self._gfa(args, 2), \
            self._gfa(args, 3)
        text = '(%s), %s %s %s' % (self.personal_name, handle, val, params)

        if self.get_property('timedelta') is not None:
            if now - self.start_time > self.get_property('timedelta'):
                self.start_time = now
                handle, val, params = self._gfa(args, 1), \
                    self._gfa(args, 2), \
                    self._gfa(args, 3)

                self.send_data(self.default_form([self.name,
                                                  self.personal_name,
                                                  handle, now_time]))
                self.default_log(text)

            return 0
        if self.get_property('delta') is not None:
            if handle is not None:
                if self.ch_prev_value is None:
                    self.ch_prev_value = self.ch_now_value

                self.ch_now_value = handle

                if self.ch_prev_value is None:
                    self.ch_prev_value = self.ch_now_value

                fl = math.fabs(self.ch_prev_value - self.ch_now_value) > \
                    self.get_property('delta')
                if self.ch_prev_value is not None and fl:
                    self.send_data(self.default_form(
                        [self.name, self.personal_name, self.ch_prev_value,
                         now_time]))

                    self.ch_prev_value = self.ch_now_value

                    self.send_data(self.default_form(
                        [self.name, self.personal_name, handle, now_time]))
                    self.default_log(text)
            return 0

        self.send_data(self.default_form(
            [self.name, self.personal_name, handle, now_time]))
        self.default_log(text)

        return 0

    def _post_init(self):
        self.ch_now_value = None
        self.ch_prev_value = None
        self.start_time = time.time()

# -*- encoding: utf-8 -*-

import datetime
import math

from PyQt4.QtCore import QObject, pyqtSignal
from src.base.monitor import CX4Monitor


class DeltaCXMonitor(CX4Monitor):
    valueToStorage = pyqtSignal(QObject, object)

    def processing(self, *args):
        now_time = str(datetime.datetime.now())
        handle, val, params = self._gfa(args, 1), self._gfa(args, 2), self._gfa(
            args, 3)

        if handle is not None:
            if self.ch_prev_value is None:
                self.ch_prev_value = self.ch_now_value
            self.ch_now_value = handle

            if self.ch_prev_value is not None and \
                            math.fabs(self.ch_prev_value - self.ch_now_value) > \
                            self.get_property('delta'):
                self.ch_prev_value = self.ch_now_value

                text = '(%s), %s %s %s' % (self.personal_name, handle, val,
                                           params)
                self.default_log(text)

                self.send_data(self.default_form([self.name,
                                                  self.personal_name, handle,
                                                  now_time]))

        return 0

    def _post_init(self):
        self.ch_now_value = None
        self.ch_prev_value = None

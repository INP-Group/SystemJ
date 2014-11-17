# -*- encoding: utf-8 -*-


import math

from src.base.channel import Channel
import datetime


class DeltaChannel(Channel):

    def processing(self, *args):
        nowtime = str(datetime.datetime.now())
        handle, val, params = self._gfa(args, 1), self._gfa(args, 2), self._gfa(args, 3)

        if not handle is None:
            if self.ch_prev_value is None:
                self.ch_prev_value = self.ch_now_value
            self.ch_now_value = handle

            if not self.ch_prev_value is None and math.fabs(self.ch_prev_value - self.ch_now_value) > self.get_property(
                    "delta"):
                self.ch_prev_value = self.ch_now_value

                text = "(%s), %s %s %s" % (self.personal_name, handle, val, params)
                self.default_log(text)

                self.send_message(self.default_form([self.name, self.personal_name, handle, nowtime]))

                # self.send_message(self.get_message(text))

        return 0

    def _post_init(self):
        self.ch_now_value = None
        self.ch_prev_value = None

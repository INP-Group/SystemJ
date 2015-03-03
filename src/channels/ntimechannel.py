# -*- encoding: utf-8 -*-


import datetime
import time

from src.base.channel import Channel


class NTimeChannel(Channel):
    def processing(self, *args):
        now = time.time()
        nowtime = str(datetime.datetime.now())
        if now - self.start_time > self.get_property("timedelta"):
            self.start_time = now
            handle, val, params = self._gfa(args, 1), \
                                  self._gfa(args, 2), \
                                  self._gfa(args, 3)
            text = "(%s), %s %s %s" % (self.personal_name, handle, val, params)

            self.default_log(text)
            self.send_message(self.default_form([self.name,
                                                 self.personal_name,
                                                 handle, nowtime]))

        return 0

    def _post_init(self):
        self.start_time = time.time()

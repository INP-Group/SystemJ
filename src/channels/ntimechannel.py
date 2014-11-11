# -*- encoding: utf-8 -*-


import time

from src.base.channel import Channel


class NTimeChannel(Channel):
    def processing(self, *args):
        now = time.time()
        if now - self.starttime > self.get_property("timedelta"):
            self.starttime = now
            handle, val, params = self._gfa(args, 1), self._gfa(args, 2), self._gfa(args, 3)
            text = "(%s), %s %s %s" % (self.personal_name, handle, val, params)

            self.default_log(text)

            self.send_message(self.get_message(text))
        return 0

    def _post_init(self):
        self.starttime = time.time()


# -*- encoding: utf-8 -*-


import datetime

from src.base.channel import Channel


class ScalarChannel(Channel):
    def processing(self, *args):
        nowtime = str(datetime.datetime.now())
        handle, val, params = self._gfa(args, 1), self._gfa(args, 2), self._gfa(args, 3)

        text = "(%s), %s %s %s" % (self.personal_name, handle, val, params)

        self.default_log(text)

        self.send_message(self.default_form([self.name, self.personal_name, handle, nowtime]))
        return 0

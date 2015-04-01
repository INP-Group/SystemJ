# -*- encoding: utf-8 -*-


import datetime

from src.base.channel import Channel


class SimpleChannel(Channel):

    def processing(self, *args):
        now_time = str(datetime.datetime.now())
        handle, val, params = self._gfa(
            args, 1), self._gfa(
            args, 2), self._gfa(
            args, 3)

        text = '(%s), %s %s %s' % (self.personal_name, handle, val, params)

        self.default_log(text)
        self.send_message(
            self.default_form([self.name, self.personal_name, handle, now_time]))
        return 0

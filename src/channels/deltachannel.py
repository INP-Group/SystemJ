# -*- encoding: utf-8 -*-


from src.base.channel import Channel

import math
import time

class DeltaChannel(Channel):

    def processing(self, *args):
        handle, val, params = self._gfa(args, 1), self._gfa(args, 2), self._gfa(args, 3)


        if not handle is None:
            if self.ch_prev_value is None:
                self.ch_prev_value = self.ch_now_value
            self.ch_now_value = handle

            if not self.ch_prev_value is None and math.fabs(self.ch_prev_value - self.ch_now_value) > self.get_property("delta"):
                self.ch_prev_value = self.ch_now_value
                text = "(%s), %s %s %s" % (self.personal_name, handle, val, params)

                with open('/home/warmonger/test.out', 'a') as f:
                    f.write("%s\n" % self.get_message(text))
                    print self.get_message(text)

        return 0

    def _post_init(self):
        self.ch_now_value = None
        self.ch_prev_value = None

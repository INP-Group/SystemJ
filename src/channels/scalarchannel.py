# -*- encoding: utf-8 -*-


from src.base.channel import Channel

class ScalarChannel(Channel):

    def processing(self, *args):
        handle, val, params = self._gfa(args, 1), self._gfa(args, 2), self._gfa(args, 3)
        print "Python Callback:", handle, val, params
        return 0
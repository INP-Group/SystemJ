# -*- encoding: utf-8 -*-


from src.base.channel import Channel

class ScalarChannel(Channel):

    def processing(self, *args):
        handle, val, params = self._gfa(args, 1), self._gfa(args, 2), self._gfa(args, 3)
        with open('/home/warmonger/test.out', 'a') as f:
            f.write("Python Callback: %s %s %s\n" % (handle, val, params))
            print "Python Callback:", handle, val, params
        return 0
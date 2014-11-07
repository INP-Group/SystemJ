# -*- encoding: utf-8 -*-


from src.base.channel import Channel

class ScalarChannel(Channel):

    def processing(self, *args):
        handle, val, params = self._gfa(args, 1), self._gfa(args, 2), self._gfa(args, 3)

        text = "(%s), %s %s %s" % (self.personal_name, handle, val, params)

        with open('/home/warmonger/test.out', 'a') as f:
            f.write("%s\n" % self.get_message(text))
            print self.get_message(text)
        return 0
# -*- encoding: utf-8 -*-


import datetime

from basedatachannel import BaseDataChannel
from src.base.zeromqchannel import ZeroMQChannel


class Channel(BaseDataChannel, ZeroMQChannel):
    def __init__(self, name, personal_name=None):
        BaseDataChannel.__init__(self, name, personal_name)
        ZeroMQChannel.__init__(self)

        self.valueChanged.connect(self.processing)

        self._post_init()

    def _post_init(self):
        None

    def get_message(self, text):
        return "[%s] [%s]: %s" % (datetime.datetime.now(), self.__class__.__name__, text)

    def default_log(self, text):
        pass
        #
        #
        # with open('/home/warmonger/test.out', 'a') as f:
        # f.write("%s\n" % self.get_message(text))
        #     print self.get_message(text)

    def default_form(self, args):
        return "\t".join(str(v).replace("'", '') for v in args)
# -*- encoding: utf-8 -*-


from basedatachannel import BaseDataChannel
from dbasechannel import DBaseChannel
import datetime

class Channel(BaseDataChannel, DBaseChannel):

    def __init__(self, name, personal_name=None):
        BaseDataChannel.__init__(self, name, personal_name)
        DBaseChannel.__init__(self)

        self.valueChanged.connect(self.processing)

        self._post_init()

    def _post_init(self):
        None

    def get_message(self, text):
        return "[%s] [%s]: %s" % (datetime.datetime.now(), self.__class__.__name__, text)
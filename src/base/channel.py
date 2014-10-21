# -*- encoding: utf-8 -*-


from basedatachannel import BaseDataChannel
from dbasechannel import DBaseChannel

class Channel(BaseDataChannel, DBaseChannel):

    def __init__(self, name):
        BaseDataChannel.__init__(self, name)
        DBaseChannel.__init__(self)

        self.valueChanged.connect(self.processing)

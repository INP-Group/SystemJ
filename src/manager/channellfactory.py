# -*- encoding: utf-8 -*-


from src.channels.scalarchannel import ScalarChannel
from src.channels.ntimechannel import NTimeChannel
from src.channels.deltachannel import DeltaChannel


class ChannelFactory(object):
    def factory(type, name, personale_name=None):
        if type == "ScalarChannel":
            return ScalarChannel(name, personale_name)
        if type == "NTimeChannel":
            return NTimeChannel(name, personale_name)
        if type == "DeltaChannel":
            return DeltaChannel(name, personale_name)
        assert 0, "Bad shape creation: " + type

    factory = staticmethod(factory)

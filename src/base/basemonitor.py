# -*- encoding: utf-8 -*-

from PyQt4.QtCore import QObject


class BaseMonitor(QObject):
    name = 'basechannel'

    def __init__(self):
        QObject.__init__(self)
        self.properties = {}

    def set_property(self, name, value):
        self.properties[name] = value

    def get_property(self, name):
        return self.properties.get(name, None)

    def _gfa(self, l, idx, default=None):
        """_get_from_args.

        :param l:
        :param idx:
        :param default:
        :return:

        """
        try:
            return l[idx]
        except IndexError:
            return default

# -*- encoding: utf-8 -*-

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *


class BaseControl(QApplication):

    def __init__(self, argv):
        super(BaseControl, self).__init__(argv)

        self.commands = {}
        self.is_debug = True

    def _add_command(self, name, func):
        self.commands[QString(name)] = func

    def _debug_on(self):
        self.is_debug = True

    def _debug_off(self):
        self.is_debug = False

    def _log(self, *args, **kwargs):
        if self.is_debug:
            print(args, kwargs)

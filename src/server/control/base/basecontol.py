# -*- encoding: utf-8 -*-

from project.settings import LOG
from project.settings import log_debug
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *


class BaseControl(QObject):
    def __init__(self):
        super(BaseControl, self).__init__()

        self.commands = {}
        self.is_debug = LOG

    def _add_command(self, name, func):
        self.commands[QString(name)] = func

    def _debug_on(self):
        self.is_debug = True

    def _debug_off(self):
        self.is_debug = False

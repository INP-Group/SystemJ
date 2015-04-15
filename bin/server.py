# -*- encoding: utf-8 -*-

try:
    from __init__ import *
except ImportError:
    pass

import sys

from project.settings import SERVER_HOST
from project.settings import SERVER_PORT
from src.server.control.controlserver import ControlServer
from PyQt4.QtCore import QCoreApplication
import signal

if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    app = QCoreApplication(sys.argv)
    form = ControlServer(SERVER_HOST, SERVER_PORT)
    sys.exit(app.exec_())

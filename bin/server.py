# -*- encoding: utf-8 -*-

try:
    from __init__ import *
except ImportError:
    pass

import signal
import sys

from project.settings import SERVER_HOST, SERVER_PORT
from PyQt4.QtCore import QCoreApplication
from src.server.control.controlserver import ControlServer

if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    app = QCoreApplication(sys.argv)
    form = ControlServer(SERVER_HOST, SERVER_PORT)
    sys.exit(app.exec_())

# -*- encoding: utf-8 -*-

try:
    from __init__ import *
except ImportError:
    pass

import sys
import signal
from project.settings import SERVER_HOST
from project.settings import SERVER_PORT
from src.server.control.manager import Manager
from PyQt4.QtCore import QCoreApplication

if __name__ == '__main__':
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    app = QCoreApplication(sys.argv)
    form = Manager(host=SERVER_HOST, port=SERVER_PORT)
    sys.exit(app.exec_())
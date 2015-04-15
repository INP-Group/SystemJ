# -*- encoding: utf-8 -*-

try:
    from __init__ import *
except ImportError:
    pass

import signal
import sys

from project.settings import SERVER_HOST
from project.settings import SERVER_PORT
from PyQt4.QtGui import QApplication
from src.server.control.guiclient import GuiClient

if __name__ == '__main__':

    signal.signal(signal.SIGINT, signal.SIG_DFL)
    app = QApplication(sys.argv)
    form = GuiClient(parent=None, host=SERVER_HOST, port=SERVER_PORT)
    form.show()
    sys.exit(app.exec_())

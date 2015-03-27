# -*- encoding: utf-8 -*-


import sys
from project.settings import SERVER_PORT, SERVER_HOST
from src.server.control.guiclient import GuiClient
from PyQt4.QtCore import *

if __name__ == '__main__':
    app = QApplication(sys.argv)
    form = GuiClient(parent=None, host=SERVER_HOST, port=SERVER_PORT)
    form.show()
    app.exec_()
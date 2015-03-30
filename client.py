# -*- encoding: utf-8 -*-


import sys

from project.settings import SERVER_HOST
from project.settings import SERVER_PORT
from PyQt4.QtGui import QApplication
from src.server.control.guiclient import GuiClient

if __name__ == '__main__':
    app = QApplication(sys.argv)
    form = GuiClient(parent=None, host=SERVER_HOST, port=SERVER_PORT)
    form.show()
    app.exec_()

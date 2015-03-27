# -*- encoding: utf-8 -*-
import sys

from project.settings import SERVER_HOST, SERVER_PORT
from src.server.control.controlserver import ControlServer

if __name__ == '__main__':
    form = ControlServer(sys.argv, SERVER_HOST, SERVER_PORT)
    form.exec_()

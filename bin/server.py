# -*- encoding: utf-8 -*-

try:
    from __init__ import *
except ImportError:
    pass

import sys

from project.settings import SERVER_HOST
from project.settings import SERVER_PORT
from src.server.control.controlserver import ControlServer

if __name__ == '__main__':
    form = ControlServer(sys.argv, SERVER_HOST, SERVER_PORT)
    form.exec_()
# -*- encoding: utf-8 -*-

import sys


from project.settings import SERVER_HOST
from project.settings import SERVER_PORT
from src.server.control.manager import Manager


if __name__ == '__main__':
    form = Manager(sys.argv, host=SERVER_HOST, port=SERVER_PORT)
    form.exec_()

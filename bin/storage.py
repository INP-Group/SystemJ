# -*- encoding: utf-8 -*-
from project.logs import log_info
from src.storage.storage import Storage

try:
    from __init__ import *
except ImportError:
    pass


def start():
    storage = Storage()
    storage.start()
    log_info('Server is stopped...')


if __name__ == '__main__':
    start()

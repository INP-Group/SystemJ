# -*- encoding: utf-8 -*-

from src.storage.storage import Storage

try:
    from __init__ import *
except ImportError:
    pass


def start():
    storage = Storage()
    storage.start()
    print('Server is stopped...')


if __name__ == '__main__':
    start()

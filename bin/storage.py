# -*- encoding: utf-8 -*-

try:
    from __init__ import *
except ImportError:
    pass

from src.storage.storage import Storage


def start():
    storage = Storage()
    storage.start()
    print('Server is stopped...')


if __name__ == '__main__':
    start()

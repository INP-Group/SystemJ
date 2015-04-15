# -*- encoding: utf-8 -*-
import os

import pickledb
from project.settings import DB_FOLDER

__all__ = [
    '_load',
    'set',
    'get'
]

def load(filepath=None, force_dump=True):
    if filepath is None:
        filepath = os.path.join(DB_FOLDER, 'kvstorage.db')
    return pickledb.load(filepath, force_dump)


def set(key, value):
    load().set(key, value)


def get(key):
    return load().get(key)


if __name__ == '__main__':
    pass

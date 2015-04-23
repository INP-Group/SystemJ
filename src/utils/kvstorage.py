# -*- encoding: utf-8 -*-
import os

import pickledb
from project.settings import DB_FOLDER

__all__ = ['_load', 'set', 'get', 'get_name_by_id', ]


def load(filepath=None, force_dump=True):
    if filepath is None:
        filepath = os.path.join(DB_FOLDER, 'kvstorage.db')
    try:
        base = pickledb.load(filepath, force_dump)
    except ValueError as e:
        os.remove(filepath)
        base = pickledb.load(filepath, force_dump)
    finally:
        return base


def set(key, value):
    load().set(key, value)


def get(key):
    return load().get(key)


def get_name_by_id(value):
    data = load()
    keys = data.getall()
    for key in keys:
        if value == data.get(key):
            return key


if __name__ == '__main__':
    pass

# -*- encoding: utf-8 -*-

try:
    from __init__ import *
except ImportError:
    pass

import os

from project.settings import POSTGRESQL_DB, POSTGRESQL_HOST, \
    POSTGRESQL_PASSWORD, POSTGRESQL_TABLE, POSTGRESQL_USER, RES_FOLDER
from src.storage.postgresql import PostgresqlStorage
from src.utils.kvstorage import set


def main():
    file_path = os.path.join(RES_FOLDER, 'channels', 'to_add.txt')
    if not os.path.exists(file_path):
        raise Exception('not found file: %s' % file_path)

    with open(file_path, 'r') as fio:
        channels = fio.readlines()

    table_name = 'channels'
    storage = PostgresqlStorage(database=POSTGRESQL_DB,
                                user=POSTGRESQL_USER,
                                password=POSTGRESQL_PASSWORD,
                                tablename=POSTGRESQL_TABLE,
                                host=POSTGRESQL_HOST)

    [storage.raw_sql('INSERT INTO %s (channel_name) VALUES (%s)' %
                     (table_name, x)) for x in channels]

    results = storage.raw_sql('SELECT * FROM %s ' % table_name)
    assert results
    for channel_id, name in results:
        set(unicode(name), channel_id)


if __name__ == '__main__':
    main()

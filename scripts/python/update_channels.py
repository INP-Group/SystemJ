# -*- encoding: utf-8 -*-

try:
    from __init__ import *
except ImportError:
    pass

import ultramemcache
from project.settings import MEMCACHE_SERVER, POSTGRESQL_DB, \
    POSTGRESQL_HOST, POSTGRESQL_PASSWORD, POSTGRESQL_TABLE, POSTGRESQL_USER
from src.storage.postgresql import PostgresqlStorage


def update_list():
    table_name = 'channels'
    storage = PostgresqlStorage(database=POSTGRESQL_DB,
                                user=POSTGRESQL_USER,
                                password=POSTGRESQL_PASSWORD,
                                tablename=POSTGRESQL_TABLE,
                                host=POSTGRESQL_HOST)

    results = storage.raw_sql('SELECT * FROM %s ' % table_name)
    assert results

    base = ultramemcache.Client([MEMCACHE_SERVER], debug=0)

    for channel_id, name in results:
        base.set(name, channel_id)


if __name__ == '__main__':
    update_list()

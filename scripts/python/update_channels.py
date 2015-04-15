# -*- encoding: utf-8 -*-

try:
    from __init__ import *
except ImportError:
    pass

from src.storage.postgresql import PostgresqlStorage
from project.settings import POSTGRESQL_DB, POSTGRESQL_USER, \
    POSTGRESQL_PASSWORD, POSTGRESQL_TABLE, POSTGRESQL_HOST

from src.utils.kvstorage import set


def update_list():
    table_name = 'channels'
    storage = PostgresqlStorage(database=POSTGRESQL_DB,
                                user=POSTGRESQL_USER,
                                password=POSTGRESQL_PASSWORD,
                                tablename=POSTGRESQL_TABLE,
                                host=POSTGRESQL_HOST)

    results = storage.raw_sql("SELECT * FROM %s " % table_name)
    assert results
    for channel_id, name in results:
        set(unicode(name), channel_id)


if __name__ == '__main__':
    update_list()
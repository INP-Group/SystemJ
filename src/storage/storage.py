# -*- encoding: utf-8 -*-
from project.settings import POSTGRESQL_DB, \
    POSTGRESQL_HOST, POSTGRESQL_PASSWORD, POSTGRESQL_TABLE, \
    POSTGRESQL_USER, ZEROMQ_HOST, ZEROMQ_PORT
from src.storage.berkeley import BerkeleyStorage
from src.storage.postgresql import PostgresqlStorage
from src.storage.zeromqserver import ZeroMQServer


class Storage(object):
    def __init__(self):
        self.sql_storage = PostgresqlStorage(database=POSTGRESQL_DB,
                                             user=POSTGRESQL_USER,
                                             password=POSTGRESQL_PASSWORD,
                                             tablename=POSTGRESQL_TABLE,
                                             host=POSTGRESQL_HOST)
        self.berkeley_db = BerkeleyStorage(sql_storage=self.sql_storage)
        self.zeromq = ZeroMQServer(host=ZEROMQ_HOST,
                                   port=ZEROMQ_PORT,
                                   berkeley_db=self.berkeley_db)

    def start(self):
        self.zeromq.start()

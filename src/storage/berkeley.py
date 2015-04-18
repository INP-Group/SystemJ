# -*- encoding: utf-8 -*-

import os
from bsddb3 import db
import ultramemcache

from project.logs import log_error
from project.settings import DB_FOLDER, MEMCACHE_SERVER
from project.settings import POSTGRESQL_DB
from project.settings import POSTGRESQL_HOST
from project.settings import POSTGRESQL_PASSWORD
from project.settings import POSTGRESQL_TABLE, BERKELEY_SYNC_NUMBER
from project.settings import POSTGRESQL_USER, BERKELEY_MOVE_NUMBER
from src.pattern.singleton import Singleton
from src.storage.postgresql import PostgresqlStorage


class GeneratorId(Singleton):
    curid = 0

    def getid(self):
        try:
            return self.curid
        finally:
            self.curid += 1

    def setId(self, id):
        self.curid = id


class BerkeleyStorage(Singleton):

    def __init__(self, filename=os.path.join(DB_FOLDER, 'berkeley.db'),
                 read=False, sql_storage=None):
        self.filename = filename
        self.database = db.DB()

        self.generator = GeneratorId()

        # todo
        # move_number == 1000
        self.sync_number = BERKELEY_SYNC_NUMBER
        self.move_number = BERKELEY_MOVE_NUMBER
        self.stored_id = 0

        self.db_type = db.DB_BTREE
        # self.db_type = db.DB_HASH
        # self.db_type = db.DB_QUEUE

        if read:
            self.database.open(self.filename, None, self.db_type,
                               db.DB_READ_COMMITTED)
        else:
            self.database.open(self.filename, None, self.db_type, db.DB_CREATE)

        if sql_storage is None:
            self.sql_storage = PostgresqlStorage(database=POSTGRESQL_DB,
                                                 user=POSTGRESQL_USER,
                                                 password=POSTGRESQL_PASSWORD,
                                                 tablename=POSTGRESQL_TABLE,
                                                 host=POSTGRESQL_HOST)
        else:
            self.sql_storage = sql_storage

        self.id = len(self.database)
        self.generator.setId(self.id)

        # todo
        # может список в динамике обновится
        self.kv_storage = ultramemcache.Client([MEMCACHE_SERVER], debug=0)

    def get_id(self, name):
        return self.kv_storage.get(name)

    def __del__(self):
        self.database.close()

    def add_json(self, data_dict):
        for key, values in data_dict.items():
            [self.add(
                '%s\t%s\t%s' % (key, val.get('value'), val.get('time'))
            )
                for val in values]

    def add(self, value):
        self.id = GeneratorId().getid()
        self.database.put(str(self.id), value)

        if not self.id % self.sync_number:
            self.database.sync()
            self.check()

    def length(self):
        return len(self.database)

    def check(self):
        """проверяет сколько уже в базе значений.

        и запускает перекладывание в postgresql
        :return:

        """

        if self.id / self.move_number > self.stored_id / self.move_number:
            values = []
            for x in xrange(self.stored_id, self.stored_id + self.move_number):
                try:
                    if self.database.get('%s' % x):
                        info = self.database.get('%s' % x).split('\t')
                        channel_id = self.get_id(info[0])
                        if channel_id is None:
                            # from scripts.python.update_channels import update_list
                            # update_list()
                            # self.kv_storage = load()
                            # channel_id = self.get_id(info[0])
                            # чтение с диска во время обработки большого потока
                            # приводит к блокировке по диску.
                            raise Exception(
                                'Not found channel_name in kvstorage')
                        values.append([info[2], info[1],
                                       channel_id])  # time, value, channel_id
                        self.database.delete('%s' % x)
                except AttributeError as e:
                    log_error(e, x, self.database.get('%s' % x))

            self.sql_storage.add(*values)

            self.stored_id += self.move_number

            return True
        return False

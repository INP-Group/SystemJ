# -*- encoding: utf-8 -*-

from bsddb3 import db

from src.base.singleton import Singleton
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

    def __init__(self, filename='berkdb.db', read=False):
        self.filename = filename
        self.database = db.DB()

        self.generator = GeneratorId()

        #todo
        # move_number == 1000
        self.sync_number = 50
        self.move_number = 100
        self.stored_id = 0

        self.db_type = db.DB_BTREE
        # self.db_type = db.DB_HASH
        # self.db_type = db.DB_QUEUE

        if read:
            self.database.open(self.filename, None, self.db_type, db.DB_READ_COMMITTED)
        else:
            self.database.open(self.filename, None, self.db_type, db.DB_CREATE)

        self.id = len(self.database)
        self.generator.setId(self.id)

        self.storage = PostgresqlStorage(user='postgres', password='147896321R')

    def __del__(self):
        self.database.close()

    def add(self, value):
        self.id = GeneratorId().getid()
        self.database.put(str(self.id), value)

        if not self.id % self.sync_number:
            self.database.sync()
            self.check()

    def length(self):
        return len(self.database)

    def check(self):
        '''
        проверяет сколько уже в базе значений.
        и запускает перекладывание в postgresql
        :return:
        '''

        if self.id / self.move_number > self.stored_id / self.move_number:
            values = []
            for x in xrange(self.stored_id, self.stored_id + self.move_number):
                try:
                    info = self.database.get("%s" % x).split("\t")
                    values.append([info[0], info[3], info[2]])
                    self.database.delete("%s" % x)
                except AttributeError as e:
                    print e, x, self.database.get("%s" % x)

            self.storage.add(*values)

            self.stored_id += self.move_number

            return True
        return False

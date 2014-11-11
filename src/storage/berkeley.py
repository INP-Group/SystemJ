# -*- encoding: utf-8 -*-

from bsddb3 import db  # the Berkeley db data base

from src.base.singleton import Singleton


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

        self.sync_number = 50
        self.move_number = 1000

        self.db_type = db.DB_BTREE
        self.db_type = db.DB_HASH

        if read:
            self.database.open(self.filename, None, self.db_type, db.DB_DIRTY_READ)
        else:
            self.database.open(self.filename, None, self.db_type, db.DB_CREATE)

        self.id = len(self.database)
        self.generator.setId(self.id)

    def __del__(self):
        self.database.close()

    def add(self, value):
        self.id = GeneratorId().getid()
        self.database.put(str(self.id), value)

        if not self.id % self.sync_number:
            self.database.sync()

    def check(self):
        '''
        проверяет сколько уже в базе значений.
        и запускает перекладывание в postgresql
        :return:
        '''
        # todo

        if not self.id % self.move_number:
            print len(self.database)
            # cursor = self.database.cursor()
            # rec = cursor.first()
            # while rec:
            #     print rec
            #     rec = cursor.next()

        self.database.close()

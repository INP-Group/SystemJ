# -*- encoding: utf-8 -*-

from src.base.singleton import Singleton

import csv
import uuid
import os
import psycopg2

class PostgresqlStorage(Singleton):

    def __init__(self, dbname='test_database', user='test_user', password='qwerty', tablename='channelsdata'):
        self.connection = psycopg2.connect(database='test_database', user='test_user', password='qwerty')

        self.database = dbname
        self.user = user
        self.password = password


        self.tablename = tablename
        self.cursor = self.connection.cursor()

    def __del__(self):
        self.cursor.close()
        self.connection.close()

    def add(self, *values):
        # temp_folder = '/tmp/temp_copy_files'
        # unique_filename = str(uuid.uuid4())
        # temp_filepath = os.path.join(temp_folder, unique_filename)
        #
        # if not os.path.exists(os.path.dirname(temp_filepath)):
        #     os.makedirs(os.path.dirname(temp_filepath))
        #
        # with open(temp_filepath, 'w') as csvfile:
        #     writer = csv.writer(csvfile)
        #     for lineValues in values:
        #         writer.writerow('\t'.join(str(v) for v in lineValues))
        #
        # fd = open(temp_filepath, 'r')
        # self.cursor.copy_from(fd, self.tablename)

        for lineValues in values:
            sql = "INSERT INTO %s VALUES (%s)" % (self.tablename, ", ".join(str(v) for v in lineValues))

            print sql
            self.cursor.execute(sql)
            # self.cursor.execute( )


        self.connection.comit()
        # fd.close()


def start():
    a = PostgresqlStorage(user='')

    values = [
        [7, "'2001-02-16 20:38:40.01'", 2.5],
        [8, "'2001-02-16 20:38:40.03'", 1.5],
        [9, "'2001-02-16 20:38:40.02'", 4.5],
        [4, "'2001-02-16 20:38:40.01'", 2.5],
        [5, "'2001-02-16 20:38:40.03'", 1.5],
        [6, "'2001-02-16 20:38:40.02'", 4.5],
    ]
    a.add(*values)

if __name__ == "__main__":
    start()
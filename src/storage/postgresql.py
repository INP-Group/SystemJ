# -*- encoding: utf-8 -*-

import os
import random
import uuid

import psycopg2
from src.base.singleton import Singleton


class PostgresqlStorage(Singleton):

    def __init__(self, database='journal_database', user='postgres',
                 password='147896321R', tablename='datachannel',
                 host='localhost'):
        self.connection = psycopg2.connect(database=database, user=user,
                                           password=password, host=host)

        self.database = database
        self.user = user
        self.password = password

        self.tablename = tablename
        self.cursor = self.connection.cursor()

    def __del__(self):
        self.cursor.close()
        self.connection.close()

    def fillrandom(self):
        def getDate():
            iso_format = "'{Year}-{Month}-{Day} {Hour}:{Minute}:{Minute}.{Offset}'"

            year_range = [str(i) for i in range(1990, 2014)]
            month_range = [str(x) for x in xrange(1, 12)]
            day_range = [str(i).zfill(2) for i in range(1, 28)]
            hour_range = [str(i).zfill(2) for i in range(1, 24)]
            min_range = [str(i).zfill(2) for i in range(1, 60)]
            offset = ['0']

            argz = {'Year': random.choice(year_range),
                    'Month': random.choice(month_range),
                    'Day': random.choice(day_range),
                    'Hour': random.choice(hour_range),
                    'Minute': random.choice(min_range),
                    'Offset': random.choice(offset),
                    }

            return iso_format.format(**argz)

        values = []
        for x in xrange(1, random.randint(0, 100)):
            values.append([str(x * random.randint(1, 12)), getDate(),
                           x * random.random() * 100])

        self.add(*values)

    def add(self, *values):

        def copy(*values):

            print 'Copy'

            temp_folder = '/tmp/temp_copy_files'
            unique_filename = str(uuid.uuid4())
            temp_filepath = os.path.join(temp_folder, unique_filename)

            if not os.path.exists(os.path.dirname(temp_filepath)):
                os.makedirs(os.path.dirname(temp_filepath))

            with open(temp_filepath, 'w') as csv_fio:
                for lineValues in values:
                    csv_fio.write('%s\n' % '\t'.join(
                        str(v).replace("'", '') for v in lineValues))

            fd = open(temp_filepath, 'r')
            self.cursor.copy_from(fd, self.tablename)
            #
            fd.close()

            os.remove(temp_filepath)

        def insert(*values):
            for lineValues in values:
                sql = 'INSERT INTO %s VALUES (%s)' % (
                    self.tablename, ', '.join(str(v) for v in lineValues))

                self.cursor.execute(sql)

        # insert(*values)
        copy(*values)
        # print values
        self.connection.commit()

    def get_data(self, channels, time_start, time_end):
        sql = "SELECT * FROM {} WHERE channel_name in ({}) AND time >= '{}' AND time <= '{}' ORDER BY time".format(
            self.tablename, ', '.join(["'%s'" % x for x in channels]),
            time_start, time_end)
        self.cursor.execute(sql)
        return self.cursor.fetchall()
# -*- encoding: utf-8 -*-

import os


def check_and_create(folder):
    if not os.path.exists(folder):
        os.makedirs(folder)


DEPLOY = True
LOG = False
PROJECT_DIR = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 os.pardir))
RES_FOLDER_NAME = 'resources'
RES_FOLDER = os.path.join(PROJECT_DIR, RES_FOLDER_NAME)

MEDIA_FOLDER = os.path.join(PROJECT_DIR, 'media')
LOG_FOLDER = os.path.join(MEDIA_FOLDER, 'logs')
DB_FOLDER = os.path.join(RES_FOLDER, 'dbs')

CDR_LIB_PATH = os.path.join(RES_FOLDER, 'libs', 'libCdr4PyQt.so')

ZEROMQ_HOST = '127.0.0.1'
ZEROMQ_PORT = '5556'

SERVER_PORT = 10000
SERVER_HOST = '127.0.0.1'

MANAGER_TEST_PORT = 10001
MANAGER_TEST_HOST = '127.0.0.1'


POSTGRESQL_HOST = 'localhost'
POSTGRESQL_DB = 'journal_database'
POSTGRESQL_TABLE = 'datachannel'
POSTGRESQL_USER = 'postgres'
POSTGRESQL_PASSWORD = '1nakopitel!'

SIZEOF_UINT32 = 4


# todo
# нужны ли такие команды?
# хреновый же API

COMMAND_SPLITER = '|||'

try:
    from project.local_settings import *
    from project.logs import *
except ImportError:
    pass

check_and_create(os.path.join(RES_FOLDER, 'plots'))
check_and_create(MEDIA_FOLDER)
check_and_create(RES_FOLDER)
check_and_create(LOG_FOLDER)
check_and_create(DB_FOLDER)



# -*- encoding: utf-8 -*-

import os


def check_and_create(folder):
    if not os.path.exists(folder):
        os.makedirs(folder)


DEPLOY = True

PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
RES_FOLDER_NAME = 'resources'
RES_FOLDER = os.path.join(PROJECT_DIR, RES_FOLDER_NAME)


MEDIA_FOLDER = os.path.join(PROJECT_DIR, 'media')
LOG_FOLDER = os.path.join(MEDIA_FOLDER, 'logs')

CDR_LIB_PATH = os.path.join(RES_FOLDER, 'libs', 'libCdr4PyQt.so')

ZEROMQ_HOST = '127.0.0.1'
ZEROMQ_PORT = '5556'

SERVER_PORT = 9090

POSTGRESQL_HOST = 'localhost'
POSTGRESQL_DB = 'journal_database'
POSTGRESQL_TABLE = 'datachannel'
POSTGRESQL_USER = 'postgres'
POSTGRESQL_PASSWORD = '1nakopitel!'


check_and_create(MEDIA_FOLDER)
check_and_create(RES_FOLDER)
check_and_create(LOG_FOLDER)

from project.local_settings import *
# -*- encoding: utf-8 -*-

import os

DEPLOY = True
LOG = False

USER = 'username'
VENV_FOLDER = 'folder'

# --- folders
PROJECT_DIR = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir))
RES_FOLDER_NAME = 'resources'
RES_FOLDER = os.path.join(PROJECT_DIR, RES_FOLDER_NAME)
MEDIA_FOLDER = os.path.join(PROJECT_DIR, 'media')
LOG_FOLDER = os.path.join(MEDIA_FOLDER, 'logs')
DB_FOLDER = os.path.join(RES_FOLDER, 'dbs')
BIN_FOLDER = os.path.join(PROJECT_DIR, 'bin')
DEPLOY_FOLDER = os.path.join(PROJECT_DIR, 'deploy')

SUPERVISORD_FOLDER = os.path.join(DEPLOY_FOLDER, 'supervisord')


# --- libs
CDR_LIB_PATH = os.path.join(RES_FOLDER, 'libs', 'libCdr4PyQt.so')

# --- others

LIST_SERVICE = ['storage', 'manager', 'server', ]

# --- params
ZEROMQ_HOST = '127.0.0.1'
ZEROMQ_PORT = '5556'

SERVER_PORT = 10000
SERVER_HOST = '127.0.0.1'

MANAGER_TEST_PORT = 10001
MANAGER_TEST_HOST = '127.0.0.1'

POSTGRESQL_HOST = ''
POSTGRESQL_DB = ''
POSTGRESQL_TABLE = ''
POSTGRESQL_USER = ''
POSTGRESQL_PASSWORD = ''

MEMCACHE_SERVER = '127.0.0.1:11211'
SIZEOF_UINT32 = 4

BERKELEY_SYNC_NUMBER = 50
BERKELEY_MOVE_NUMBER = 200

# todo
# нужны ли такие команды?
# хреновый же API

COMMAND_SPLITER = '|||'

try:
    from project.logs import *
    from project.functions import *
    from project.local_settings import *
except ImportError as e:
    print(e)


VENV_ACTIVATE = os.path.join(VENV_FOLDER, 'bin/activate')
VENV_PYTHON = os.path.join(VENV_FOLDER, 'bin/python')

# --- create folders

folders = [MEDIA_FOLDER,
           RES_FOLDER,
           LOG_FOLDER,
           DB_FOLDER,
           os.path.join(RES_FOLDER, 'plots'),
           BIN_FOLDER,
           DEPLOY_FOLDER,
           SUPERVISORD_FOLDER, ]

[check_and_create(x) for x in folders]

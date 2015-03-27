# -*- encoding: utf-8 -*-

import os


def check_and_create(folder):
    if not os.path.exists(folder):
        os.makedirs(folder)


DEPLOY = True

PROJECT_DIR = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)),
                 os.pardir))
RES_FOLDER_NAME = 'resources'
RES_FOLDER = os.path.join(PROJECT_DIR, RES_FOLDER_NAME)

MEDIA_FOLDER = os.path.join(PROJECT_DIR, 'media')
LOG_FOLDER = os.path.join(MEDIA_FOLDER, 'logs')
DB_FOLDER = os.path.join(RES_FOLDER, 'dbs')


check_and_create(MEDIA_FOLDER)
check_and_create(RES_FOLDER)
check_and_create(LOG_FOLDER)
check_and_create(DB_FOLDER)



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




#todo
# нужны ли такие команды?
# хреновый же API

COMMAND_SPLITER = "|||"

MANAGER_COMMAND = {
    "TEST": "test",
}

SERVER_MANAGER_COMMAND = {
    "ONLINE": "Manager online",
    "OFFLINE": "Manager offline"
}

SERVER_YET_COMMAND = {
    "HI": "Hello world",
    "MANAGER_CNT": "How many managers?",
    "MANAGER_LIST": "I want to get all the information about the managers",
    "CHANNEL_CNT": "How many channels?",
    "CHANNEL_LIST": "I want to get all the information about the channels",
    "CHANNEL_ADD": "Please, add new channel(s)",
}


def get_text_command(command):
    if command in MANAGER_COMMAND:
        return MANAGER_COMMAND.get(command)
    if command in SERVER_MANAGER_COMMAND:
        return SERVER_MANAGER_COMMAND.get(command)
    if command in SERVER_YET_COMMAND:
        return SERVER_YET_COMMAND.get(command)

from project.local_settings import *
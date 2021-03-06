# -*- encoding: utf-8 -*-
import sys

from project.logs import log_error
from project.settings import COMMAND_SPLITER
from PyQt4.QtCore import QString
from src.server.control.base.baseserver import BaseServer


class ControlServer(BaseServer):
    def __init__(self, host, port):
        super(ControlServer, self).__init__(host, port)

        self._add_command('USER_LIST', self._command_users)
        self._add_command('MANAGER_LIST', self._command_managers)
        self._add_command('SET_TYPE', self._command_set_type)
        self._add_command('CHL_LIST', self._command_users)
        self._add_command('FROM_FILE', self._command_from_file)

        self.manager_commands = ['CHL_ADD',
                                 'WORKER_ADD',
                                 'WORKER_DEL',
                                 'WORKER_LIST', ]

        [self._add_command(x, self._command_send_to_manager)
         for x in self.manager_commands]

    def _command_managers(self, client, command, message):
        assert self.users
        data = str([info for key, info in self.users.items()
                    if info.get('type') == 'manager'])
        [self.send_message(x, 'RAW', data) for x, info in self.users.items()
         if info.get('type') == 'guiclient']

    def _command_set_type(self, client, command, message):
        assert self.users
        self.users[client]['type'] = str(message)
        if str(message) == 'manager':
            self.users[client]['cnt_monitors'] = 0
            self.users[client]['cnt_worker'] = 0
        self.send_message(client, 'SET')

    def _command_send_to_manager(self, client, command, message):
        def minimum_with_key(users, key='cnt_monitors'):
            min_cnt = sys.maxsize
            min_client = users[0][0]
            for client, info in users:
                if info.get(key) < min_cnt:
                    min_cnt = info.get(key)
                    min_client = client
            return min_client

        managers = [(client, info) for client, info in self.users.items()
                    if info.get('type') == 'manager']

        if len(managers) == 1:
            real_manager = managers[0][0]
            real_manager_info = managers[0][0]
        elif len(managers) == 0:
            raise Exception('Not found managers')
        else:
            if command == 'CHL_ADD':
                real_manager = minimum_with_key(managers, 'cnt_monitors')
            elif command == 'WORKER_ADD':
                real_manager = minimum_with_key(managers, 'cnt_worker')
            else:
                real_manager = managers[0][0]

        self.send_message(real_manager, command, message)

        if command == 'CHL_ADD':
            self.users[real_manager]['cnt_monitors'] += 1
        if command == 'WORKER_ADD':
            self.users[real_manager]['cnt_worker'] += 1

    def _command_channel_add(self, client, command, message):
        # todo
        # Добавить возможность добавлять в определенный менеджер

        for client, info in self.users.items():
            if info.get('type') == 'manager':
                self.send_message(client, 'CHL_ADD', message)
                break

    def _command_from_file(self, client, command, message):
        commands = message.split('\n')
        commands = list(filter(lambda x: COMMAND_SPLITER in x, commands))
        for command in commands:
            command, message = [y.strip() for y in str(command).split('|||')]
            if command == 'SET_TYPE':
                self._command_set_type(client, command, message)
            elif QString(command) in self.manager_commands:
                self._command_send_to_manager(client, command, message)
            elif QString(command) in self.commands:
                self.commands.get(QString(command))(client, command, message)
            else:
                log_error('Not found command', command, message)

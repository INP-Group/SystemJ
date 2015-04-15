# -*- encoding: utf-8 -*-
from project.logs import log_error

from project.settings import COMMAND_SPLITER
from PyQt4.QtCore import QString
from src.server.control.base.baseserver import BaseServer


class ControlServer(BaseServer):

    def __init__(self, argv, host, port):
        super(ControlServer, self).__init__(argv, host, port)

        self._add_command('USER_LIST', self._command_users)
        self._add_command('MANAGER_LIST', self._command_managers)
        self._add_command('SET_TYPE', self._command_set_type)
        self._add_command('CHL_LIST', self._command_users)
        self._add_command('FROM_FILE', self._command_from_file)

        self.manager_commands = [
            'CHL_ADD',
            'WORKER_ADD',
            'WORKER_DEL',
            'WORKER_LIST',
        ]

        [self._add_command(x, self._command_send_to_manager)
         for x in self.manager_commands]

    def _command_managers(self, client, command, message):
        assert self.users
        data = str(
            [info for key, info in self.users.items() if
             info.get('type') == 'manager'])
        [self.send_message(x, 'RAW', data) for x, info in self.users.items() if
         info.get(
             'type') == 'guiclient']

    def _command_set_type(self, client, command, message):
        assert self.users
        self.users[client]['type'] = str(message)
        self.send_message(client, 'SET')

    def _command_send_to_manager(self, client, command, message):
        for client, info in self.users.items():
            if info.get('type') == 'manager':
                self.send_message(client, command, message)
                break

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

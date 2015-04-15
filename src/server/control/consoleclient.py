# -*- encoding: utf-8 -*-

from src.server.control.base.baseclient import BaseClient
from project.settings import log_error, log_debug


class ConsoleClient(BaseClient):

    def __init__(self, argv, host, port, client_name=None):
        super(ConsoleClient, self).__init__(argv, host, port, client_name=None)

        self.server_host = host
        self.server_port = port
        if client_name is None:
            self.client_name = 'Client_name'
        else:
            self.client_name = client_name

        self.nextBlockSize = 0
        self.request = None
        self.firstTime = True
        self._init_socket()

        self.commands = {}
        self._add_command('ECHO', self._command_echo)
        self._add_command('OFFLINE', self._command_off)

        self.connect_server()

    # Create connection to server
    def connect_server(self):
        self.socket.connectToHost(self.server_host, self.server_port)
        if self.firstTime:
            self.send_message('ONLINE', self.client_name)
            self.firstTime = False

    def server_has_error(self):
        self.send_message('OFFLINE', '')
        log_error('Error: {}'.format(
            self.socket.errorString()))
        self.socket.close()
        self.quit()

    def _command_echo(self, command, message):
        log_debug('ECHO (command): %s' % message)

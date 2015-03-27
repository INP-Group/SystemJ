# -*- encoding: utf-8 -*-

import sys

from project.settings import SERVER_HOST
from project.settings import SERVER_PORT
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *
from src.server.control.base.baseclient import BaseClient


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
        print('Error: {}'.format(
            self.socket.errorString()))
        self.socket.close()
        self.quit()

    def _command_echo(self, command, message):
        self._log('ECHO (command): %s' % message)


if __name__ == '__main__':
    form = ConsoleClient(sys.argv, host=SERVER_HOST, port=SERVER_PORT)
    form.exec_()

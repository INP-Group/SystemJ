# -*- encoding: utf-8 -*-

import socket
from project.settings import SERVER_PORT, SERVER_HOST, COMMAND_SPLITER


class BaseSocketClient(object):
    def __init__(self, host=SERVER_HOST, port=SERVER_PORT):
        self.host = host
        self.port = port
        self.socket = None

    def _send(self, command, value=None):
        if self.socket is None:
            self._connection()

        try:
            self.socket.sendall(
                "{} {} {}\n".format(command, COMMAND_SPLITER, value))
            received = self.socket.recv(1024)
        finally:
            self._close()
        return received

    def _connection(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((self.host, self.port))

    def _close(self):
        self.socket.close()
        self.socket = None
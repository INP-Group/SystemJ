# -*- encoding: utf-8 -*-

from project.settings import SIZEOF_UINT32
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *
from src.server.control.base.basecontol import BaseControl


class BaseClient(BaseControl):

    def __init__(self, argv, host, port, client_name=None):
        super(BaseClient, self).__init__(argv)

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

    def _init_socket(self):
        # Ititialize socket
        self.socket = QTcpSocket()

        # Signals and slots for networking
        self.socket.readyRead.connect(self.read_server)
        self.socket.disconnected.connect(self.server_has_stopped)
        self.connect(self.socket,
                     SIGNAL('error(QAbstractSocket::SocketError)'),
                     self.server_has_error)

    # Create connection to server
    def connect_server(self):
        self.socket.connectToHost(self.server_host, self.server_port)
        if self.firstTime:
            self.send_message('ONLINE', self.client_name)
            self.firstTime = False

    def server_has_stopped(self):
        self.socket.close()

    def server_has_error(self):
        self._log('Error: {}'.format(
            self.socket.errorString()))
        self.socket.close()
        self.quit()

    def send_message(self, command, message=''):
        self.request = QByteArray()
        stream = QDataStream(self.request, QIODevice.WriteOnly)
        stream.setVersion(QDataStream.Qt_4_2)
        stream.writeUInt32(0)

        stream << QString(command) << QString(message)

        stream.device().seek(0)
        stream.writeUInt32(self.request.size() - SIZEOF_UINT32)
        self.socket.write(self.request)
        self.nextBlockSize = 0
        self.request = None

    def read_server(self):
        stream = QDataStream(self.socket)
        stream.setVersion(QDataStream.Qt_4_2)

        while True:
            if self.nextBlockSize == 0:
                if self.socket.bytesAvailable() < SIZEOF_UINT32:
                    break
                self.nextBlockSize = stream.readUInt32()
            if self.socket.bytesAvailable() < self.nextBlockSize:
                break
            command = QString()
            message = QString()
            stream >> command >> message
            self._log(
                'RECEIVE: command - %s, message - %s' % (command, message))
            self.process_message(command, message)

            self.nextBlockSize = 0

    def process_message(self, command, message):
        if command in self.commands:
            self.commands[command](message, command)

    def _command_echo(self, command, message):
        self._log('ECHO (command): %s' % message)

    def _command_off(self, command, message):
        self.socket.close()

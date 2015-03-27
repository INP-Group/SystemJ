# -*- encoding: utf-8 -*-

import sys

import project.settings
from project.settings import SIZEOF_UINT32
from src.server.control.base.basecontol import BaseControl

if not project.settings.DEPLOY:
    import DLFCN

    old_dlopen_flags = sys.getdlopenflags()
    sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)
    from PyQt4.QtCore import *
    from PyQt4.QtGui import *
    from PyQt4.QtNetwork import *

    sys.setdlopenflags(old_dlopen_flags)
else:
    from PyQt4.QtCore import *
    from PyQt4.QtGui import *
    from PyQt4.QtNetwork import *


class BaseServer(BaseControl):

    def __init__(self, argv, host, port):
        super(BaseServer, self).__init__(argv)

        self.tcp_server = QTcpServer(self)

        if not self.tcp_server.listen(QHostAddress(host), port):
            self._log('Unable to start the server: {0}.'.format(
                self.tcp_server.errorString()))
            return

        self._log(
            'The server is running on port {0}.'.format(
                self.tcp_server.serverPort())
            + '\nRun the Fortune Client example now.')

        self.connect(self.tcp_server, SIGNAL('newConnection()'),
                     self.new_connection)
        self.connections = []
        self.users = {}

        self.commands = {}
        self._add_command('ONLINE', self._command_online)
        self._add_command('ECHO', self._command_echo)
        self._add_command('USERS', self._command_users)

    def send_message(self, client, command, message=''):
        self._log(
            'SEND COMMAND (server): command %s, message %s' %
            (command, message))
        reply = QByteArray()
        stream = QDataStream(reply, QIODevice.WriteOnly)
        stream.setVersion(QDataStream.Qt_4_2)
        stream.writeUInt32(0)
        stream << QString(command) << QString(message)
        stream.device().seek(0)
        stream.writeUInt32(reply.size() - SIZEOF_UINT32)
        client.write(reply)

    def remove_connection(self):
        pass

    def socket_error(self):
        pass

    def new_connection(self):
        client_connection = self.tcp_server.nextPendingConnection()
        client_connection.nextBlockSize = 0
        self.connections.append(client_connection)
        self.connect(client_connection, SIGNAL('readyRead()'),
                     self.receive_message)
        self.connect(client_connection, SIGNAL('disconnected()'),
                     self.remove_connection)
        self.connect(client_connection, SIGNAL('error()'),
                     self.socket_error)

    def receive_message(self):
        for s in self.connections:
            if s.bytesAvailable() > 0:
                stream = QDataStream(s)
                stream.setVersion(QDataStream.Qt_4_2)

                if s.nextBlockSize == 0:
                    if s.bytesAvailable() < SIZEOF_UINT32:
                        return
                    s.nextBlockSize = stream.readUInt32()
                if s.bytesAvailable() < s.nextBlockSize:
                    return

                action = QString()
                message = QString()
                stream >> action >> message

                self.process_message(s, action, message)

    def process_message(self, client, command, message):
        client.nextBlockSize = 0

        self._log('RECEIVE: command: %s, message: %s' % (command, message))
        if command in self.commands:
            self.commands[command](client, command, message)

        client.nextBlockSize = 0

    def _command_users(self, client, command, message):
        assert self.users
        self.send_message(client, 'RAW', str(self.users))

    def _command_online(self, client, command, message):
        if message not in self.users:
            self.users[client] = {
                'name': str(message),
                'status': 'online',
                'type': 'unknown',
                'socket': client,
            }
            self.send_message(client, 'HI', 'Hi %s' % message)
        else:
            self.send_message(client, 'BAD', 'Name already exist')

    def _command_echo(self, client, command, message):
        self._log(
            'ECHO (command): Client: {}, command: {}, message: {}'.format(
                client.socketDescriptor(),
                command, message)
        )

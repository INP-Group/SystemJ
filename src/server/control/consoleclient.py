# -*- encoding: utf-8 -*-

import sys
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *
from project.settings import SERVER_PORT, SERVER_HOST

PORT = SERVER_PORT
SIZEOF_UINT32 = 4


class ConsoleClient(QObject):
    def __init__(self):
        super(ConsoleClient, self).__init__()

        self.commands = {
            QString("ECHO"): self._command_echo,
            QString("OFFLINE"): self._command_off,
        }

        self.client_name = "Client_name"
        # Initialize data IO variables
        self.nextBlockSize = 0
        self.request = None
        self.firstTime = True
        self._init_socket()

        self.connect_server()

    def _init_socket(self):
        # Ititialize socket
        self.socket = QTcpSocket()

        # Signals and slots for networking
        self.socket.readyRead.connect(self.read_server)
        self.socket.disconnected.connect(self.server_has_stopper)
        self.connect(self.socket,
                     SIGNAL("error(QAbstractSocket::SocketError)"),
                     self.server_has_error)

    # Create connection to server
    def connect_server(self):
        self.socket.connectToHost("localhost", PORT)
        if self.firstTime:
            self.send_message("ONLINE", self.client_name)
            self.firstTime = False

    def server_has_stopper(self):
        self.socket.close()

    def server_has_error(self):
        print("Error: {}".format(
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
            print(command, message)
            self.process_message(command, message)

            self.nextBlockSize = 0

    def process_message(self, command, message):
        if command in self.commands:
            self.commands[command](message, command)

    def _command_echo(self, message, command=None):
        print(message)

    def _command_off(self, message, command=None):
        self.socket.close()
        # def process_command(self, command, message):
        # if command == "CHAT":
        #         self.updateUi(message)
        #     elif command == "AUTH":
        #         self.updateUi(message)
        #         self.firstTime = True
        #     elif command == "OK":
        #         self.send_message("SET_TYPE", "manager")
        #     elif command == "SET":
        #         print('okk')


if __name__ == '__main__':
    app = QApplication(sys.argv)
    form = ConsoleClient()
    app.exec_()
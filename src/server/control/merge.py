# -*- encoding: utf-8 -*-

import sys
import datetime
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *
from project.settings import SERVER_HOST, SERVER_PORT

PORT = 9999
SIZEOF_UINT32 = 4


class Merge(QObject):
    def __init__(self, parent=None):
        super(Merge, self).__init__()

        self.tcpServer = QTcpServer(self)

        if not self.tcpServer.listen(QHostAddress(SERVER_HOST),
                                     PORT):
            print("Unable to start the server: {0}.".format(
                self.tcpServer.errorString()))
            return

        print(
            "The server is running on port {0}.".format(
                self.tcpServer.serverPort())
            + "\nRun the Fortune Client example now.")

        self.connect(self.tcpServer, SIGNAL("newConnection()"),
                     self.addConnection)
        self.connections = []
        self.users = {}

        # client

        # Initialize data IO variables
        self.nextBlockSize = 0
        self.request = None
        self.firstTime = True


        self.socket = QTcpSocket()
        self.socket.readyRead.connect(self.readFromServer)
        self.socket.disconnected.connect(self.serverHasStopped)
        self.connect(self.socket,
                     SIGNAL("error(QAbstractSocket::SocketError)"),
                     self.serverHasError)

        self.connectToServer()


    # Create connection to server
    def connectToServer(self):
        self.socket.connectToHost("localhost", SERVER_PORT)

    def send_message(self, command, message):
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

    def readFromServer(self):
        stream = QDataStream(self.socket)
        stream.setVersion(QDataStream.Qt_4_2)

        while True:
            if self.nextBlockSize == 0:
                if self.socket.bytesAvailable() < SIZEOF_UINT32:
                    break
                self.nextBlockSize = stream.readUInt32()
            if self.socket.bytesAvailable() < self.nextBlockSize:
                break
            action = QString()
            textFromServer = QString()
            stream >> action >> textFromServer
            print(action, textFromServer)
            if action == "CHAT":
                pass
            elif action == "TEST":
                self.send_message("LALALALA", "Hi maaaan!")
            elif action == "AUTH":
                pass
                self.firstTime = True
            self.nextBlockSize = 0

    def serverHasStopped(self):
        self.socket.close()
        self.connectButton.setEnabled(True)

    def serverHasError(self):
        self.updateUi("Error: {}".format(
            self.socket.errorString()))
        self.socket.close()
        self.connectButton.setEnabled(True)


    def auth(self, s):
        reply = QByteArray()
        stream = QDataStream(reply, QIODevice.WriteOnly)
        stream.setVersion(QDataStream.Qt_4_2)
        stream.writeUInt32(0)
        stream << QString("AUTH") << QString("<p>What's you name?</p>")
        stream.device().seek(0)
        stream.writeUInt32(reply.size() - SIZEOF_UINT32)
        s.write(reply)

    def addConnection(self):
        clientConnection = self.tcpServer.nextPendingConnection()
        clientConnection.nextBlockSize = 0
        self.connections.append(clientConnection)
        self.connect(clientConnection, SIGNAL("readyRead()"),
                     self.receiveMessage)
        self.connect(clientConnection, SIGNAL("disconnected()"),
                     self.removeConnection)
        self.connect(clientConnection, SIGNAL("error()"),
                     self.socketError)
        self.sendAuth(clientConnection)


    def receiveMessage(self):
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
                textFromClient = QString()
                stream >> action >> textFromClient
                print(action, textFromClient)
                if action == "SEND":
                    s.nextBlockSize = 0

                    self.sendMessage(textFromClient,
                                     s.socketDescriptor())
                    s.nextBlockSize = 0
                elif action == "AUTH":
                    s.nextBlockSize = 0
                    self.authMsg(s, textFromClient)
                    s.nextBlockSize = 0

    def sendAuth(self, s):
        reply = QByteArray()
        stream = QDataStream(reply, QIODevice.WriteOnly)
        stream.setVersion(QDataStream.Qt_4_2)
        stream.writeUInt32(0)
        message = QString("<p>Manager</p>")
        stream << QString("AUTH") << QString(message)
        stream.device().seek(0)
        stream.writeUInt32(reply.size() - SIZEOF_UINT32)
        s.write(reply)

    def authMsg(self, s, text):

        reply = QByteArray()
        stream = QDataStream(reply, QIODevice.WriteOnly)
        stream.setVersion(QDataStream.Qt_4_2)
        stream.writeUInt32(0)
        if self.users.has_key(text):
            stream << QString("AUTH") << QString(
                text + " is taken,please change one.")
        else:
            self.users[text] = s
            stream << QString("CHAT") << QString(
                text + ",Great,now you can chat!")
        stream.device().seek(0)
        stream.writeUInt32(reply.size() - SIZEOF_UINT32)
        s.write(reply)

    def sendMessage(self, text, socketId):
        now = datetime.datetime.now()
        for user in self.users:
            if self.users[user].socketDescriptor() == socketId:
                sender = user
                break
        for user in self.users:
            if self.users[user].socketDescriptor() == socketId:
                message = "<p>" + str(now.strftime(
                    "%Y-%m-%d %H:%M:%S")) + "</p>" + "<font color=red>You</font> > {}".format(
                    text)
            else:
                message = "<p>" + str(now.strftime(
                    "%Y-%m-%d %H:%M:%S")) + "</p>" + "<font color=blue>" + sender + "</font>" + " > {}".format(
                    text)
            reply = QByteArray()
            stream = QDataStream(reply, QIODevice.WriteOnly)
            stream.setVersion(QDataStream.Qt_4_2)
            stream.writeUInt32(0)
            stream << QString("CHAT") << QString(message)
            stream.device().seek(0)
            stream.writeUInt32(reply.size() - SIZEOF_UINT32)
            self.users[user].write(reply)

    def removeConnection(self):
        pass

    def socketError(self):
        pass


if __name__ == '__main__':
    app = QApplication(sys.argv)
    form = Merge()
    app.exec_()
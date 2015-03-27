# -*- encoding: utf-8 -*-

import sys
import datetime
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *
from project.settings import SERVER_HOST, SERVER_PORT

PORT = 9999
SIZEOF_UINT32 = 4


class ServerDlg(QObject):
    def __init__(self, parent=None):
        super(ServerDlg, self).__init__()

        ## server

        self.tcpServer = QTcpServer(self)

        if not self.tcpServer.listen(QHostAddress(SERVER_HOST), SERVER_PORT):
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
                else:
                    s.nextBlockSize = 0
                    self.sendMessage2(action, textFromClient,
                                     s.socketDescriptor())
                    s.nextBlockSize = 0

    def sendAuth(self, s):
        reply = QByteArray()
        stream = QDataStream(reply, QIODevice.WriteOnly)
        stream.setVersion(QDataStream.Qt_4_2)
        stream.writeUInt32(0)
        message = QString("<p>Connected,please input your username</p>")
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

    def sendMessage2(self, action, message, socketId):
        now = datetime.datetime.now()
        for user in self.users:
            if self.users[user].socketDescriptor() == socketId:
                sender = user
                break
        for user in self.users:
            reply = QByteArray()
            stream = QDataStream(reply, QIODevice.WriteOnly)
            stream.setVersion(QDataStream.Qt_4_2)
            stream.writeUInt32(0)
            stream << QString(action) << QString(message)
            stream.device().seek(0)
            stream.writeUInt32(reply.size() - SIZEOF_UINT32)
            self.users[user].write(reply)


    def removeConnection(self):
        pass

    def socketError(self):
        pass


if __name__ == '__main__':
    app = QApplication(sys.argv)
    form = ServerDlg()
    app.exec_()
# -*- encoding: utf-8 -*-

import random
from PyQt4 import QtCore, QtNetwork

from PyQt4.QtCore import QCoreApplication
from PyQt4.QtNetwork import QHostAddress
from project.settings import SERVER_HOST, SERVER_PORT


class MessageServer(QCoreApplication):
    def __init__(self, parent=None):
        QCoreApplication.__init__(self, parent)

        self.server = QtNetwork.QTcpServer(self)
        if not self.server.listen(QHostAddress(SERVER_HOST), SERVER_PORT):
            print("Unable to start the server: {0}.".format(
                self.server.errorString()))
            return

        print(
            "The server is running on port {0}.".format(
                self.server.serverPort())
            + "\nRun the Fortune Client example now.")

        self.fortunes = (
            "You've been leading a dog's life. Stay off the furniture.",
            "You've got to think about tomorrow.",
            "You will be surprised by a loud noise.",
            "You will feel hungry again in another hour.",
            "You might have mail",
            "You cannot kill time without injuring eternity.",
            "Computers are not intelligent. They only think they are.")

        self.server.newConnection.connect(self.sendFortune)

    def sendFortune(self):
        block = QtCore.QByteArray()
        out = QtCore.QDataStream(block, QtCore.QIODevice.WriteOnly)
        out.setVersion(QtCore.QDataStream.Qt_4_0)
        out.writeUInt16(0)
        fortune = self.fortunes[random.randint(0, len(self.fortunes) - 1)]

        try:
            # python3
            fortune = bytes(fortune, encoding="ascii")
        except Exception:
            # python2
            pass

        out.writeString(fortune)
        out.device().seek(0)
        out.writeUInt16(block.size() - 2)

        clientConnection = self.server.nextPendingConnection()
        clientConnection.disconnected.connect(clientConnection.deleteLater)

        clientConnection.write(block)
        clientConnection.disconnectFromHost()


if __name__ == "__main__":
    import sys

    app = MessageServer([])

    sys.exit(app.exec_())
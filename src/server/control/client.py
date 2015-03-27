# -*- encoding: utf-8 -*-

import sys
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *
from project.settings import SERVER_PORT, SERVER_HOST

PORT = SERVER_PORT
SIZEOF_UINT32 = 4

class Form(QDialog):

    def __init__(self, parent=None):
        super(Form, self).__init__(parent)

        # Ititialize socket
        self.socket = QTcpSocket()

        # Initialize data IO variables
        self.nextBlockSize = 0
        self.request = None
        self.firstTime = True

        # Create widgets/layout
        self.browser = QTextBrowser()
        self.lineedit = QLineEdit("Enter text here, dummy")
        self.lineedit.selectAll()
        self.connectButton = QPushButton("Connect")
        self.connectButton.setEnabled(True)
        layout = QVBoxLayout()
        layout.addWidget(self.browser)
        layout.addWidget(self.lineedit)
        layout.addWidget(self.connectButton)
        self.setLayout(layout)
        self.lineedit.setFocus()

        # Signals and slots for line edit and connect button
        self.lineedit.returnPressed.connect(self.issueRequest)
        self.connectButton.clicked.connect(self.connectToServer)

        self.setWindowTitle("Client")
        # Signals and slots for networking
        self.socket.readyRead.connect(self.readFromServer)
        self.socket.disconnected.connect(self.serverHasStopped)
        self.connect(self.socket,
                     SIGNAL("error(QAbstractSocket::SocketError)"),
                     self.serverHasError)

    # Update GUI
    def updateUi(self, text):
        self.browser.append(text)

    # Create connection to server
    def connectToServer(self):
        self.connectButton.setEnabled(False)
        self.socket.connectToHost("localhost", PORT)

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

    def issueRequest(self):
        if self.firstTime:
            self.send_message("AUTH", self.lineedit.text())
            self.firstTime = False
        else:
            self.send_message("TEST", self.lineedit.text())
        self.lineedit.setText("")

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
                self.updateUi(textFromServer)
            elif action =="AUTH":
                self.updateUi(textFromServer)
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


if __name__ == '__main__':

    app = QApplication(sys.argv)
    form = Form()
    form.show()
    app.exec_()
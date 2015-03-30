# -*- encoding: utf-8 -*-

import datetime

from project.settings import COMMAND_SPLITER
from project.settings import SIZEOF_UINT32
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtNetwork import *


class GuiClient(QDialog):

    def __init__(self, parent=None, host='localhost', port='10000'):
        super(GuiClient, self).__init__(parent)

        self.commands = {
            QString('ECHO'): self._command_echo,
            QString('RAW'): self._command_echo,
            QString('OFFLINE'): self._command_off,
        }

        self.nextBlockSize = 0
        self.request = None
        self.firstTime = True
        self.server_host = host
        self.server_port = port

        self.commands = {}
        self.is_debug = True

        self._add_command('ECHO', self._command_echo)
        self._add_command('RAW', self._command_echo)
        self._add_command('OFFLINE', self._command_off)
        self._add_command('HI', self._command_set_type)

        self._init_socket()
        self._init_gui()

        width = 630
        height = 500

        self.setMinimumSize(width, height)
        self.resize(width, height)

        self.center_on_screen()

    def center_on_screen(self):
        """centerOnScreen() Centers the window on the screen."""
        resolution = QDesktopWidget().screenGeometry()
        self.move((resolution.width() / 2) - (self.frameSize().width() / 2),
                  (resolution.height() / 2) - (self.frameSize().height() / 2))

    def _add_command(self, name, func):
        self.commands[QString(name)] = func

    def _init_socket(self):
        self.socket = QTcpSocket()

        # Signals and slots for networking
        self.socket.readyRead.connect(self.read_server)
        self.socket.disconnected.connect(self.server_has_stopped)
        self.connect(self.socket,
                     SIGNAL('error(QAbstractSocket::SocketError)'),
                     self.server_has_error)

    def _init_gui(self):
        # Create widgets/layout
        self.TE_browser = QTextBrowser()
        self.LE_text = QLineEdit('MyClientName')
        self.LE_text.selectAll()
        self.PB_Connect = QPushButton('Connect')
        self.PB_Connect.setEnabled(True)
        layout = QVBoxLayout()
        layout.addWidget(self.TE_browser)
        layout.addWidget(self.LE_text)
        layout.addWidget(self.PB_Connect)
        self.setLayout(layout)
        self.LE_text.setFocus()

        # Signals and slots for line edit and connect button
        self.LE_text.returnPressed.connect(self.send_request)
        self.PB_Connect.clicked.connect(self.connect_server)
        self.setWindowTitle('Client')

        # for debug
        self.connect_server()

    # Update GUI
    def update_gui(self, text):
        now = str(datetime.datetime.now().strftime(
            '%Y-%m-%d %H:%M:%S'))
        message = '<p>{}</p> <font color=red>You</font> > {}'.format(
            now, text
        )
        self.TE_browser.append(message)

        # Create connection to server

    def connect_server(self):
        self.PB_Connect.setEnabled(False)
        self.socket.connectToHost(self.server_host, self.server_port)
        self.send_request()

    def send_message(self, command, message):
        self.request = QByteArray()
        stream = QDataStream(self.request, QIODevice.WriteOnly)
        stream.setVersion(QDataStream.Qt_4_2)
        stream.writeUInt32(0)

        self._log('SEND: command %s, message %s' % (command, message))
        stream << QString(command) << QString(message)

        stream.device().seek(0)
        stream.writeUInt32(self.request.size() - SIZEOF_UINT32)
        self.socket.write(self.request)
        self.nextBlockSize = 0
        self.request = None

    def _debug_on(self):
        self.is_debug = True

    def _debug_off(self):
        self.is_debug = False

    def _log(self, *args, **kwargs):
        if self.is_debug:
            print(args, kwargs)

    def server_has_stopped(self):
        self.socket.close()
        self.PB_Connect.setEnabled(True)

    def server_has_error(self):
        self.update_gui('Error: {}'.format(
            self.socket.errorString()))
        self.socket.close()
        self.PB_Connect.setEnabled(True)

    def send_request(self):
        if self.firstTime:
            self.send_message('ONLINE', self.LE_text.text())
            self.firstTime = False
        else:
            message = self.LE_text.text()
            if COMMAND_SPLITER in message:
                command, message = [str(x).strip() for x in
                                    message.split(COMMAND_SPLITER)]
            else:
                command, message = 'ECHO', self.LE_text.text()
            self.update_gui('%s : %s' % (command, message))
            self.send_message(command, message)
        self.LE_text.setText('')

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
            self._log('RECEIVE: command: %s, message:%s' % (command, message))

            self.process_message(command, message)
            self.nextBlockSize = 0

    def process_message(self, command, message):
        if command in self.commands:
            self.commands[command](command, message)

    def _command_echo(self, command, message):
        self.update_gui(message)

    def _command_off(self, command, message):
        self.socket.close()

    def _command_set_type(self, command, message):
        self.send_message('SET_TYPE', 'guiclient')

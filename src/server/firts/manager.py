# -*- encoding: utf-8 -*-


# -*- encoding: utf-8 -*-


import socket

from src.server.base import commands

class ManageObj(object):
    def __init__(self, port, host='localhost'):
        self.host = host
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((self.host, port))

        self.socket.send(self.form_message("IM_MANAGER", "None"))

        while True:
            data = self.socket.recv(16)
            print data

    def form_message(self, command, text):
        sep = "-" * 10
        text = "%s\n%s\n%s" % (commands.get(command, "None"), sep, text)
        return text

        #
        #
        # if key == 'ADD_CH':
        #
        # if key == 'ADD_CH_TO':
        #
        # if key == 'REM_CH_FROM':
        #
        # if key == 'SET_PROP':
        #

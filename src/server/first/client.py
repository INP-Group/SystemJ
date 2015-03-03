# -*- encoding: utf-8 -*-


import socket

from src.server.base import commands


class ClientManager(object):
    def __init__(self, port, host='localhost'):
        self.host = host
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((self.host, port))

    def form_message(self, command, text):
        sep = "-" * 10
        text = "%s\n%s\n%s" % (commands.get(command, "None"), sep, text)
        return text

    def send(self, command, *args):
        text = self.form_message(command, str(args).strip('[]'))

        self.socket.send(text)

# -*- encoding: utf-8 -*-

from src.server.make.baseclient import BaseSocketClient
from project.settings import get_text_command


class ClientManager(BaseSocketClient):
    def __init__(self, server_host, server_port, manager_host, manager_port):
        super(ClientManager, self).__init__(server_host, server_port)
        self.manager_port = manager_port
        self.manager_host = manager_host

    def send_online(self):
        result = self._send(get_text_command("ONLINE"),
                            {'host': self.manager_host,
                             'port': self.manager_port})
        print(result)

    def send_offline(self):
        result = self._send(get_text_command("OFFLINE"),
                            {'host': self.manager_host,
                             'port': self.manager_port})
        print(result)

# -*- encoding: utf-8 -*-

from src.server.make.baseclient import BaseSocketClient
from project.settings import MANAGER_COMMAND as command


class ClientManager(BaseSocketClient):

    def send_online(self):
        result = self._send(command.get("ONLINE"))
        print(result)

    def send_offline(self):
        result = self._send(command.get("OFFLINE"))
        print(result)

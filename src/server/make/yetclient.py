# -*- encoding: utf-8 -*-


from project.settings import YET_COMMAND as command
from src.server.make.baseclient import BaseSocketClient


class YetClient(BaseSocketClient):

    def send_online(self):
        result = self.send(command.get("ONLINE"))
        print(result)

    def send_offline(self):
        result = self.send(command.get("OFFLINE"))
        print(result)

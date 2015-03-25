# -*- encoding: utf-8 -*-


from project.settings import SERVER_YET_COMMAND as command
from src.server.make.baseclient import BaseSocketClient


class YetClient(BaseSocketClient):
    def send_hi(self):
        result = self._send(command.get("HI"))
        print(result)

    def get_manager_cnt(self):
        result = self._send(command.get("MANAGER_CNT"))
        print(result)

    def get_manager_list(self):
        result = self._send(command.get("MANAGER_LIST"))
        print(result)

    def get_channel_cnt(self):
        result = self._send(command.get("CHANNEL_CNT"))
        print(result)

    def get_channel_list(self):
        result = self._send(command.get("CHANNEL_LIST"))
        print(result)

    def set_channels(self, channels):
        result = self._send(command.get("CHANNEL_ADD"), channels)
        print(result)

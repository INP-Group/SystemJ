# -*- encoding: utf-8 -*-

from src.server.make.baseclient import BaseSocketClient
from project.settings import SERVER_MANAGER_COMMAND as command, \
    MANAGER_TEST_HOST, MANAGER_TEST_PORT


class ClientManager(BaseSocketClient):

    def send_online(self):
        result = self._send(command.get("ONLINE"), {'host': MANAGER_TEST_HOST,
                                                    'port': MANAGER_TEST_PORT})
        print(result)

    def send_offline(self):
        result = self._send(command.get("OFFLINE"), {'host': MANAGER_TEST_HOST,
                                                     'port': MANAGER_TEST_PORT})
        print(result)

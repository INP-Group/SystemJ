# -*- encoding: utf-8 -*-

import SocketServer
from project.settings import MANAGER_COMMAND, COMMAND_SPLITER


class ThreadedBlockManager(SocketServer.ThreadingMixIn,
                           SocketServer.TCPServer):
    pass


class BlockManager(SocketServer.BaseRequestHandler):
    def handle(self):
        # self.request is the TCP socket connected to the client
        data = self.request.recv(1024).strip()
        request_command, request_value = [x.strip() for x in
                                          data.split(COMMAND_SPLITER)]

        command = []
        if request_command in MANAGER_COMMAND.values():
            command = [key for key, value in MANAGER_COMMAND.items() if
                       request_command == value]

        assert len(command) == 1
        result = self.process_command(command[0], request_value)

        self.request.sendall(result)

    def process_command(self, command, value):
        # todo
        # hindi processing
        result = "Unknown command (in processing)"
        if command == "TEST":
            result = {'ok': True, 'result': "TEST message"}

        return self._to_string(result)

    @staticmethod
    def _to_string(value):
        return str(value)

    def add_channel(self):
        pass


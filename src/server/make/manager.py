# -*- encoding: utf-8 -*-

import SocketServer




from project.settings import MANAGER_COMMAND, COMMAND_SPLITER
from src.channels.scalarchannel import ScalarChannel
from src.channels.simplechannel import SimpleChannel


class ThreadedBlockManager(SocketServer.ThreadingMixIn,
                           SocketServer.TCPServer):
    pass


class BlockManager(SocketServer.BaseRequestHandler):
    channels = []

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
            self.add_channel()
            result = {'ok': True, 'result': "TEST message"}

        return self._to_string(result)

    @staticmethod
    def _to_string(value):
        return str(value)

    def add_channel(self):
        self.channels.append(SimpleChannel("linthermcan.ThermosM.in0"))


# -*- encoding: utf-8 -*-

import SocketServer
from project.settings import MANAGER_COMMAND, COMMAND_SPLITER


class ServerManger(SocketServer.BaseRequestHandler):
    # todo
    # сохранять надо в БД
    info_dict = {}

    def handle(self):
        # self.request is the TCP socket connected to the client
        data = self.request.recv(1024).strip()
        command, value = [x.strip() for x in data.split(COMMAND_SPLITER)]
        client_ip, client_port = self.client_address[0], self.client_address[1]

        client = "%s:%s" % (client_ip, client_port)
        if self.info_dict.get(client, None) is None:
            self.info_dict[client] = {"status": "unknown"}

        if command in MANAGER_COMMAND.values():
            command = [key for key, value in MANAGER_COMMAND.items() if
                       command == value]
            assert len(command) == 1
            result = self.process_command(command[0], value, client)
        else:
            result = "Unknown command"

        self.request.sendall(result)

    def process_command(self, command, value, client):
        result = "Unknown command (in processing)"
        if command == "ONLINE":
            self.info_dict[client]["status"] = "online"
            result = "Manager is online"
        elif command == "OFFLINE":
            self.info_dict[client]["status"] = "offline"
            result = "Manage is offline"

        return result

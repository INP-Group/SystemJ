# -*- encoding: utf-8 -*-

import SocketServer
from project.settings import MANAGER_COMMAND, COMMAND_SPLITER, YET_COMMAND


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
            self.info_dict[client] = {"status": "unknown", "type": "unknown"}

        if command in MANAGER_COMMAND.values():
            command = [key for key, value in MANAGER_COMMAND.items() if
                       command == value]

        if command in YET_COMMAND.values():
            command = [key for key, value in YET_COMMAND.items() if
                       command == value]

        assert len(command) == 1
        result = self.process_command(command[0], value, client)

        self.request.sendall(result)

    def process_command(self, command, value, client):
        # todo
        # hindi processing
        result = "Unknown command (in processing)"
        if command == "ONLINE":
            self.info_dict[client]["status"] = "online"
            self.info_dict[client]["type"] = "manager"
            result = {'ok': True, 'result': "Manager is online"}

        elif command == "OFFLINE":
            self.info_dict[client]["status"] = "offline"
            result = {'ok': True, 'result': "Manage is offline"}

        elif command == "HI":
            result = {'ok': True, 'result': "Hi yetclient, what's up?"}

        elif command == "MANAGER_CNT":
            try:
                cnt = sum([1 for x in self.info_dict.values() if
                           x.get('status') == 'online' and x.get(
                               'type') == 'manager'])
                result = {'ok': True, 'result': cnt}
            except Exception as e:
                result = {'ok': False, 'error': str(e)}

        elif command == "MANAGER_LIST":
            try:
                info = [(x, value) for x, value in self.info_dict.items() if
                        value.get(
                            'type') == 'manager']
                result = {'ok': True, 'result': info}
            except Exception as e:
                result = {'ok': False, 'error': str(e)}

        elif command == "CHANNEL_CNT":
            try:
                cnt = sum([1 for x in self.info_dict.values() if x.get(
                    'type') == 'channel'])
                result = {'ok': True, 'result': cnt}
            except Exception as e:
                result = {'ok': False, 'error': str(e)}

        elif command == "CHANNEL_LIST":
            try:
                info = [(x, value) for x, value in self.info_dict.items() if
                        value.get(
                            'type') == 'channel']
                result = {'ok': True, 'result': info}
            except Exception as e:
                result = {'ok': False, 'error': str(e)}

        elif command == "CHANNEL_ADD":

            result = {'ok': False, 'error': "Ok (todo)"}
        return self._to_string(result)

    @staticmethod
    def _to_string(value):
        return str(value)



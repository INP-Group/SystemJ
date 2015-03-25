# -*- encoding: utf-8 -*-

import SocketServer
from project.settings import SERVER_MANAGER_COMMAND, COMMAND_SPLITER, \
    SERVER_YET_COMMAND, get_text_command
from src.server.make.utils import send_message


class ThreadedServerManager(SocketServer.ThreadingMixIn,
                            SocketServer.TCPServer):
    pass


class ServerManager(SocketServer.BaseRequestHandler):
    # todo
    # сохранять надо в БД
    info_dict = {}

    def add_client(self, host, port):
        client = "%s:%s" % (host, port)
        if self.info_dict.get(client, None) is None:
            self.info_dict[client] = {"status": "unknown", "type": "unknown"}

        return client

    def handle(self):
        # self.request is the TCP socket connected to the client
        data = self.request.recv(1024).strip()
        request_command, request_value = [x.strip() for x in
                                          data.split(COMMAND_SPLITER)]

        client = self.add_client(self.client_address[0], self.client_address[1])

        command = []
        if request_command in SERVER_MANAGER_COMMAND.values():
            command = [key for key, value in SERVER_MANAGER_COMMAND.items() if
                       request_command == value]

        elif request_command in SERVER_YET_COMMAND.values():
            command = [key for key, value in SERVER_YET_COMMAND.items() if
                       request_command == value]

        assert len(command) == 1
        result = self.process_command(command[0], request_value, client)

        self.request.sendall(result)

    def process_command(self, command, value, client):
        # todo
        # hindi processing

        #print(command, value)
        result = "Unknown command (in processing)"
        if command == "ONLINE":
            info = eval(value)
            manager_host = info.get('host')
            manager_port = info.get('port')
            self.add_client(manager_host, manager_port)
            manager_info = "%s:%s" % (manager_host, manager_port)
            self.info_dict[manager_info]["status"] = "online"
            self.info_dict[manager_info]["type"] = "manager"
            result = {'ok': True, 'result': "Manager is online"}

        elif command == "OFFLINE":
            info = eval(value)
            manager_host = info.get('host')
            manager_port = info.get('port')
            manager_info = "%s:%s" % (manager_host, manager_port)
            self.info_dict[manager_info]["status"] = "offline"
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
            info = [(x, value) for x, value in self.info_dict.items() if
                    value.get(
                        'type') == 'manager' and value.get(
                        'status') == 'online']
            # todo
            # может не быть менеджеров в онлайне
            host, port = info[0][0].split(':')
            result = send_message(host=host, port=int(port), command=get_text_command('TEST'),
                               client_type='manager', value=None)
        return self._to_string(result)

    @staticmethod
    def _to_string(value):
        return str(value)



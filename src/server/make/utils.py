# -*- encoding: utf-8 -*-
from src.server.make.clientmanager import ClientManager
from src.server.make.yetclient import YetClient


def send_message(host, port, command, client_type='manager', value=None):
    if client_type == 'manager':
        client = ClientManager(host, port, host, port)
    elif client_type == 'yet':
        client = YetClient(host=host, port=port)
    else:
        raise Exception("Not found type of client")
    result = client._send(command, value)
    return result
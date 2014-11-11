# -*- encoding: utf-8 -*-
from src.storage.zeromqserver import ZeroMQServer


def start():

    port = '5678'
    host = '127.0.0.1'

    server = ZeroMQServer(host=host, port=port)
    server.start()


if __name__ == '__main__':
    start()
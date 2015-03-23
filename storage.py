# -*- encoding: utf-8 -*-
from src.storage.zeromqserver import ZeroMQServer
from settings import ZEROMQ_PORT, ZEROMQ_HOST

def start():
    server = ZeroMQServer(host=ZEROMQ_HOST, port=ZEROMQ_PORT)
    server.start()

    print("Server is stopped...")


if __name__ == '__main__':
    start()

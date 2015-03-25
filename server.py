# -*- encoding: utf-8 -*-
import SocketServer

from src.server.make.server import Server
from project.settings import SERVER_PORT, SERVER_HOST


def start():
    # server = ServerManager(SERVER_PORT)
    # server.run()

    # Create the server, binding to localhost on port 9999
    server = SocketServer.TCPServer((SERVER_HOST, SERVER_PORT), Server)
    server.serve_forever()


if __name__ == "__main__":
    start()

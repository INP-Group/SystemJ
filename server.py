# -*- encoding: utf-8 -*-


from src.server.first.server import ServerManager
from settings import SERVER_PORT


def start():
    server = ServerManager(SERVER_PORT)
    server.run()


if __name__ == "__main__":
    start()

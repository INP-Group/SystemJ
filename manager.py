# -*- encoding: utf-8 -*-

from src.server.manager import ManageObj

def start():
    PORT = 9090
    a = ManageObj(PORT)


if __name__ == "__main__":
    start()
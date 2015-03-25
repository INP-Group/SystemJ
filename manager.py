# -*- encoding: utf-8 -*-
import threading
from src.server.make.clientmanager import ClientManager

from src.server.make.manager import BlockManager, ThreadedBlockManager
from project.settings import MANAGER_TEST_PORT, MANAGER_TEST_HOST, SERVER_PORT, SERVER_HOST


def start():
    server = ThreadedBlockManager((MANAGER_TEST_HOST, MANAGER_TEST_PORT),
                                  BlockManager)

    client = ClientManager(server_host=SERVER_HOST, server_port=SERVER_PORT,
                       manager_host=MANAGER_TEST_HOST,
                       manager_port=MANAGER_TEST_PORT)

    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    print("Start (manager) server")
    server_thread.start()

    try:
        client.send_online()
        while True:
            pass
    except KeyboardInterrupt:
        print("Stop (manager) server")
        client.send_offline()
        server.shutdown()


if __name__ == "__main__":
    start()

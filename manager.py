# -*- encoding: utf-8 -*-
import threading

from src.server.make.manager import BlockManager, ThreadedBlockManager
from project.settings import MANAGER_TEST_PORT, MANAGER_TEST_HOST


def start():
    server = ThreadedBlockManager((MANAGER_TEST_HOST, MANAGER_TEST_PORT),
                                  BlockManager)
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    print("Start (manager) server")
    server_thread.start()

    try:
        while True:
            pass
    except KeyboardInterrupt:
        print("Stop (manager) server")
        server.shutdown()


if __name__ == "__main__":
    start()

# -*- encoding: utf-8 -*-
import threading

from src.server.make.servermanager import ThreadedServerManager, ServerManager
from project.settings import SERVER_PORT, SERVER_HOST


def start():
    server = ThreadedServerManager((SERVER_HOST, SERVER_PORT), ServerManager)
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    print("Start server")
    server_thread.start()

    try:
        while True:
            pass
    except KeyboardInterrupt:
        print("Stop server")
        server.shutdown()

    #
    # server = SocketServer.TCPServer((SERVER_HOST, SERVER_PORT), ServerManger)
    #
    # try:
    #     print("Start server")
    #     server.serve_forever()
    # except KeyboardInterrupt as e:
    #     print("Stop server")
    # except Exception as e:
    #     print("Stop server")
    # finally:
    #     server.server_close()
    #     server.shutdown()
    #


if __name__ == "__main__":
    start()

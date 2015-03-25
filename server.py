# -*- encoding: utf-8 -*-
import SocketServer

from src.server.make.servermanager import ServerManger
from project.settings import SERVER_PORT, SERVER_HOST


def start():
    server = SocketServer.TCPServer((SERVER_HOST, SERVER_PORT), ServerManger)

    try:
        print("Start server")
        server.serve_forever()
    except KeyboardInterrupt as e:
        print("Stop server")
    except Exception as e:
        print("Stop server")
    finally:
        server.server_close()
        server.shutdown()


if __name__ == "__main__":
    start()

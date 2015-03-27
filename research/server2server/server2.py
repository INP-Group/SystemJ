# -*- encoding: utf-8 -*-
import socket
import threading
import SocketServer
import time


class Server2(SocketServer.BaseRequestHandler):

    def handle(self):
        data = self.request.recv(1024)
        cur_thread = threading.current_thread()
        response = '(server2) {}: {}'.format(cur_thread.name, data)
        self.request.sendall(response)


class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass


def client(ip, port, message):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((ip, port))
    try:
        sock.sendall(message)
        response = sock.recv(1024)
        print 'Received: {}'.format(response)
    finally:
        sock.close()

if __name__ == '__main__':
    # Port 0 means to select an arbitrary unused port
    HOST, PORT = 'localhost', 10001
    port2 = 10000

    server = ThreadedTCPServer((HOST, PORT), Server2)
    ip, port = server.server_address

    # Start a thread with the server -- that thread will then start one
    # more thread for each request
    server_thread = threading.Thread(target=server.serve_forever)
    # Exit the server thread when the main thread terminates
    server_thread.daemon = True
    server_thread.start()
    print 'Server loop running in thread:', server_thread.name

    client(ip, port, 'Hello World 1')
    client(ip, port, 'Hello World 2')
    client(ip, port, 'Hello World 3')

    i = 0
    time.sleep(5)
    while True:
        try:
            i += 1
            if i % 1000000 == 0:
                i = 0
                client(ip, port2, 'Hi from server2')
        except KeyboardInterrupt:
            break
    server.shutdown()

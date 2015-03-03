# -*- encoding: utf-8 -*-

import socket
import sys
import threading


class LoggingServer(threading.Thread):
    def __init__(self, port, host='localhost'):
        threading.Thread.__init__(self)
        self.port = port
        self.host = host
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.users = {}  # current connections

        try:
            self.server.bind((self.host, self.port))
        except socket.error:
            print('Bind failed %s' % (socket.error))
            sys.exit()

        self.server.listen(10)

    # Not currently used. Ensure sockets are closed on disconnect
    def exit(self):
        self.server.close()

    def __del__(self):
        for user, con in self.users.items():
            con.close()
        self.server.close()

    def run_thread(self, conn, addr):
        print('Client connected with ' + addr[0] + ':' + str(addr[1]))

        input_data = ""
        while True:
            data = conn.recv(1024)
            if data:
                input_data += data
                reply = b'OK...' + data
                print(reply)
            else:
                break
            # conn.sendall(reply)



    def run(self):
        print('Waiting for connections on port %s' % self.port)
        # We need to run a loop and create a new thread for each connection
        while True:
            conn, addr = self.server.accept()

            threading.Thread(target=self.run_thread, args=(conn, addr)).start()

# -*- encoding: utf-8 -*-

import zmq
import threading

class ZeroMQServer(threading.Thread):

    def __init__(self, host='127.0.0.1', port='5678'):
        threading.Thread.__init__(self)
        self.context = zmq.Context()
        self.sock = self.context.socket(zmq.REP)
        self.sock.bind("tcp://%s:%s" % (host, port))

    def start(self):
        while True:
            message = self.sock.recv()
            self.sock.send("Echo: " + message)
            print "Echo: " + message

def start():
    port = '5678'
    host = '127.0.0.1'

    server = ZeroMQServer(host=host, port=port)
    server.start()


if __name__ == '__main__':

    start()
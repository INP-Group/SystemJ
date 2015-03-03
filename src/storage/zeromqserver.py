# -*- encoding: utf-8 -*-

import threading

import zmq

from src.storage.berkeley import BerkeleyStorage


class ZeroMQServer(threading.Thread):
    def __init__(self, host='127.0.0.1', port='5678'):
        threading.Thread.__init__(self)
        self.context = zmq.Context()
        self.sock = self.context.socket(zmq.REP)
        self.sock.bind("tcp://%s:%s" % (host, port))

        self.berkldb = BerkeleyStorage()

    def start(self):
        try:
            while True:
                message = self.sock.recv()
                self.berkldb.add(message)
                self.sock.send("Saved")
        except KeyboardInterrupt:
            self.berkldb.__del__()
            self.sock.close()

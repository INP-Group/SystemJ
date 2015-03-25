# -*- encoding: utf-8 -*-

import threading

import zmq
from src.storage.berkeley import BerkeleyStorage
from settings import ZEROMQ_HOST, ZEROMQ_PORT


class ZeroMQServer(threading.Thread):
    def __init__(self, host=ZEROMQ_HOST, port=ZEROMQ_PORT):
        threading.Thread.__init__(self)
        self.context = zmq.Context()
        self.sock = self.context.socket(zmq.REP)
        self.sock.bind("tcp://%s:%s" % (host, port))

        self.berkeley_db = BerkeleyStorage()

    def start(self):
        try:
            print("Server is started...")
            while True:
                message = self.sock.recv()
                if message:
                    # print("Receive message: %s " % message)
                    self.berkeley_db.add(message)
                    self.sock.send("Saved")
        except KeyboardInterrupt:
            self.berkeley_db.__del__()
            self.sock.close()

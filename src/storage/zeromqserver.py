# -*- encoding: utf-8 -*-

import threading

import zmq
from project.settings import LOG
from project.settings import ZEROMQ_HOST
from project.settings import ZEROMQ_PORT
from project.logs import log_debug
from project.logs import log_info
from src.storage.berkeley import BerkeleyStorage


class ZeroMQServer(threading.Thread):

    def __init__(self, host=ZEROMQ_HOST, port=ZEROMQ_PORT, berkeley_db=None):
        threading.Thread.__init__(self)
        self.context = zmq.Context()
        self.sock = self.context.socket(zmq.REP)
        self.sock.bind('tcp://%s:%s' % (host, port))

        if berkeley_db is None:
            self.berkeley_db = BerkeleyStorage()
        else:
            self.berkeley_db = berkeley_db

    def start(self):
        try:
            log_info('Server is started...')
            while True:
                message = self.sock.recv()
                if message:
                    if LOG:
                        log_debug('Receive message: %s ' % message)
                    self.berkeley_db.add(message)
                    self.sock.send('Saved')
        except KeyboardInterrupt:
            self.stop()

    def stop(self):
        self.berkeley_db.__del__()
        self.sock.close()

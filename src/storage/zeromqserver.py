# -*- encoding: utf-8 -*-

import threading

import zmq
from project.logs import log_debug, log_info
from project.settings import LOG, ZEROMQ_HOST, ZEROMQ_PORT
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
                json_data = self.sock.recv_json()
                if json_data:
                    if LOG:
                        log_debug('Receive message: %s, type: %s' %
                                  (json_data, type(json_data)))
                    self.berkeley_db.add_json(json_data)
                    self.sock.send('Saved')
        except KeyboardInterrupt:
            self.stop()

    def stop(self):
        self.berkeley_db.__del__()
        self.sock.close()

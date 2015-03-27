# -*- encoding: utf-8 -*-

import zmq
from basechannel import BaseChannel
from project.settings import ZEROMQ_HOST
from project.settings import ZEROMQ_PORT


class ZeroMQChannel(BaseChannel):

    def __init__(self, host=ZEROMQ_HOST, port=ZEROMQ_PORT):
        super(ZeroMQChannel, self).__init__()
        self.host = host
        self.port = port

        self.attempt = 0
        self.max_attempt = 10

    def store_data(self, text):
        # todo
        # реализовать складирование данных в файл
        pass

    def send_message(self, text):
        # init zeromqsocket
        if self.attempt >= self.max_attempt:
            self.store_data(text)
            return

        self.context = zmq.Context()
        self.sock = self.context.socket(zmq.REQ)
        self.sock.connect('tcp://%s:%s' % (self.host, self.port))

        self.sock.send_string(text)
        answer = self.sock.recv()
        if answer.strip() == 'Saved':
            self.sock.close()
            self.attempt = 0
            return
        else:
            self.attempt += 1
            self.send_message(text)

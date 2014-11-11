# -*- encoding: utf-8 -*-

import zmq

from basechannel import BaseChannel


class ZeroMQChannel(BaseChannel):
    def __init__(self, host='127.0.0.1', port='5678'):
        super(ZeroMQChannel, self).__init__()

        # init zeromqsocket
        self.context = zmq.Context()
        self.sock = self.context.socket(zmq.REQ)
        self.sock.connect("tcp://%s:%s" % (host, port))

    def send_message(self, text):
        self.sock.send_string(text)
        answer = self.sock.recv()
        if answer.strip() == "Saved":
            return
        else:
            self.send_message(text)
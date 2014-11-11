# -*- encoding: utf-8 -*-

from basechannel import BaseChannel
import zmq

class ZeroMQChannel(BaseChannel):

    def __init__(self, host='127.0.0.1', port='5678'):
        super(ZeroMQChannel, self).__init__()

        # init zeromqsocket
        self.context = zmq.Context()
        self.sock = self.context.socket(zmq.REQ)
        self.sock.connect("tcp://%s:%s" % (host, port))

    def send_message(self, text):
        self.sock.send_string(text)
        self.sock.recv()
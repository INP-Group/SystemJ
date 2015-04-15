# -*- encoding: utf-8 -*-

import zmq
from project.settings import ZEROMQ_HOST
from project.settings import ZEROMQ_PORT
from PyQt4.QtCore import QObject
from PyQt4.QtCore import pyqtSignal
from src.base.basemonitor import BaseMonitor


class ZeroMQMonitor(BaseMonitor):

    valueToStorage = pyqtSignal(QObject, object)

    def __init__(self, host=ZEROMQ_HOST, port=ZEROMQ_PORT):
        super(ZeroMQMonitor, self).__init__()
        self.host = host
        self.port = port

        self.attempt = 0
        self.max_attempt = 10

    def store_data(self, text):
        # todo
        # реализовать складирование данных в файл
        pass

    def send_data(self, json_data):
        self.valueToStorage.emit(self, json_data)
        # # init zeromqsocket
        # if self.attempt >= self.max_attempt:
        #     self.store_data(text)
        #     return
        #
        # self.context = zmq.Context()
        # self.sock = self.context.socket(zmq.REQ)
        # self.sock.connect('tcp://%s:%s' % (self.host, self.port))
        #
        # self.sock.send_string(text)
        # answer = self.sock.recv()
        # if answer.strip() == 'Saved':
        #     self.sock.close()
        #     self.attempt = 0
        #     return
        # else:
        #     self.attempt += 1
        #     self.send_message(text)

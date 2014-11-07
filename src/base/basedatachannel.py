# -*- encoding: utf-8 -*-


from basechannel import BaseChannel

from PyQt4.QtCore import *
import math
from cdr_wrapper import Cdr


class BaseDataChannel(BaseChannel):

    valueChanged = pyqtSignal(QObject, float)
    valueMeasured = pyqtSignal(QObject, float)

    def __init__(self, name):
        QObject.__init__(self)
        #todo сделать конфиг, чтобы хотя бы указывать путь до .so файда
        #todo hardcode
        self.name = name
        self.cdr = Cdr('/home/warmonger/Dropbox/Study/Diploma/Diploma/resources/libs/libCdr4PyQt.so')
        # self.cdr = Cdr()
        self.cx_chan = self.cdr.RegisterSimpleChan(name, self.callback)

        self.prev_val = 0.0  # value to compare with just received
        self.val = 0.0
        self.tolerance = 0.0
        self.first_cycle = True



    def processing(self, *args):
        raise NotImplemented("Callback")

    # the callback for cdr
    def callback(self, handle, val, params):
        self.val = val
        if math.fabs(self.val - self.prev_val) > self.tolerance or self.first_cycle:
            self.first_cycle = False
            self.prev_val = self.val
            self.valueChanged.emit(self, self.val)
        self.valueMeasured.emit(self, self.val)

        return 0

    def setValue(self, value):
        self.cdr.SetSimpleChanVal(self.cx_chan, value)

    def setTolerance(self, tolerance):
        self.tolerance = tolerance


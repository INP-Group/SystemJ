from PyQt4.QtGui import *
from PyQt4.QtCore import *

from actl import *


class FLCDNumber(QLCDNumber):
    def __init__(self, parent=None):
        QLCDNumber.__init__(self, parent)


class FLCDNumberCX(QLCDNumber):
    cxname = pyqtProperty(str)

    # 'sukhpanel.s-m.ch0'
    def __init__(self, parent=None):
        QLCDNumber.__init__(self, parent)
        #self.cxchan = cxchan(self.cxname)
        #self.cxchan.valueMeasured.connect(self.display)
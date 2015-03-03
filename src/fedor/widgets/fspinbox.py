from PyQt4.QtCore import *
from PyQt4.QtGui import *


class FSpinBox(QSpinBox):
    done = pyqtSignal(int)

    def __init__(self, parent=None):
        QSpinBox.__init__(self, parent)
        self.valueChanged.connect(self.done)

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class FDoubleSpinBox(QDoubleSpinBox):
    done = pyqtSignal(float)

    def __init__(self, parent=None):
        QDoubleSpinBox.__init__(self, parent)
        self.valueChanged.connect(self.done)

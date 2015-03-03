from PyQt4.QtCore import *
from PyQt4.QtGui import *


class FCheckBox(QCheckBox):
    done = pyqtSignal(bool)

    def __init__(self, parent=None):
        QCheckBox.__init__(self, parent)
        self.clicked.connect(self.done)

    def setValue(self, state):
        self.setChecked(state)

    def value(self):
        return self.isChecked()

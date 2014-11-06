from PyQt4.QtGui import *
from PyQt4.QtCore import *

class FComboBox(QComboBox):

    done = pyqtSignal(int)

    def __init__(self, parent=None):
        QComboBox.__init__(self, parent)
        self.currentIndexChanged.connect(self.done)

    def setValue(self, ind):
        self.setCurrentIndex(ind)

    def value(self):
        return self.currentIndex()
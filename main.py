# -*- encoding: utf-8 -*-


from src.channels.scalarchannel import ScalarChannel
from src.channels.vectorchannel import VectorChannel
from src.manager.manager import WorkerManager


import sys

from PyQt4 import QtCore, QtGui


class MainWindow(QtGui.QMainWindow):
    def __init__(self):
          QtGui.QMainWindow.__init__(self)

          self.setWindowTitle('What')

          button = QtGui.QPushButton('Quit')
          button.setFont(QtGui.QFont("Times", 10, QtGui.QFont.Bold))
          self.connect(button, QtCore.SIGNAL('clicked()'), QtCore.SLOT('close()'))

          self.setCentralWidget(button)

def start():
    application = QtGui.QApplication(sys.argv)
    mainwidow = MainWindow()
    mainwidow.show()

    scal = ScalarChannel("linthermcan.ThermosM.in0")

    sys.exit(application.exec_())


def start3():
    application = QtGui.QApplication(sys.argv)
    win = WorkerManager()
    win.show()
    sys.exit(application.exec_())

if __name__ == "__main__":
    start()
    # start3()



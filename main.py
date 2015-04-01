# -*- encoding: utf-8 -*-

import sys

import project.settings
from research.manager.manager import WorkerManager
from src.channels.simplemonitor import SimpleMonitor

if not project.settings.DEPLOY:
    import DLFCN
    old_dlopen_flags = sys.getdlopenflags()
    sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)
    from PyQt4 import QtCore, QtGui
    sys.setdlopenflags(old_dlopen_flags)
else:
    from PyQt4 import QtCore, QtGui


class MainWindow(QtGui.QMainWindow):

    def __init__(self):
        QtGui.QMainWindow.__init__(self)

        self.setWindowTitle('What')

        button = QtGui.QPushButton('Quit')
        button.setFont(QtGui.QFont('Times', 10, QtGui.QFont.Bold))
        self.connect(
            button,
            QtCore.SIGNAL('clicked()'),
            QtCore.SLOT('close()'))

        self.setCentralWidget(button)


def start():
    application = QtGui.QApplication(sys.argv)
    mainwidow = MainWindow()
    mainwidow.show()

    # scal = ScalarMonitor("linvac.vacmatrix.Imes0")
    scal = SimpleMonitor('linthermcan.ThermosM.in0')

    sys.exit(application.exec_())


def start3():
    application = QtGui.QApplication(sys.argv)
    win = WorkerManager()
    win.show()

    sys.exit(application.exec_())

if __name__ == '__main__':
    start()
    # start3()

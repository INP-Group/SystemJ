# -*- encoding: utf-8 -*-

from src.channels.scalarchannel import ScalarChannel

from PyQt4 import QtCore, QtGui
import time


'''
Создать worker'а, который будет внутри содержать объект класса скалярного канала
По клику добавлять объекты в воркера
'''
class Testobject2 (QtGui.QWidget):
    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)

        self.buttonDaemon = QtGui.QPushButton(self)
        self.buttonDaemon2 = QtGui.QPushButton(self)
        self.layout = QtGui.QVBoxLayout(self)
        self.layout.addWidget(self.buttonDaemon)
        self.layout.addWidget(self.buttonDaemon2)
        self.setLayout(self.layout)

        self.thread = DaemonWorker(name="Daemon1")

        self.connect(self.buttonDaemon, QtCore.SIGNAL('clicked()'), self.pressDaemon)
        self.connect(self.buttonDaemon2, QtCore.SIGNAL('clicked()'), self.addChannell)


    def addChannell(self):
        self.thread.addchanel()

    # the problem begins below: I'm not using signals, or queue, or whatever, while I believe I should for StopSignal and DaemonRunning
    def pressDaemon (self):

        # self.buttonDaemon.setEnabled(False)
        if self.thread.isDaemonRunning():
            self.thread.setDaemonStopSignal(True)
            self.buttonDaemon.setText('Daemon - run code every %s sec'% 1)
        else:
            self.thread.startDaemon()
            self.buttonDaemon.setText('Stop Daemon')
            self.buttonDaemon.setEnabled(True)

    # part of proposed solution
    def stopped (self, val):
        print 'stopped1 ' + str(val)

class DaemonWorker(QtCore.QThread):
    daemonIsRunning = False
    daemonStopSignal = False
    daemonCurrentDelay = 0

    def isDaemonRunning (self): return self.daemonIsRunning
    def setDaemonStopSignal (self, bool): self.daemonStopSignal = bool

    def __init__ (self, parent = None, name='Daemon1'):
        QtCore.QThread.__init__(self, parent)
        self.exiting = False
        self.name = name
        self.thread_to_run = None
        self.channels = []

    def __del__ (self):
        self.exiting = True
        self.thread_to_run = None
        self.wait()

    def run (self):
        if self.thread_to_run != None:
            self.thread_to_run(mode='continue')


    def addchanel(self):
        self.channels.append(ScalarChannel("linthermcan.ThermosM.in0"))

    #stop = QtCore.pyqtSignal(int) # part of proposed solution

    def startDaemon (self, mode = 'run'):
        if mode == 'run':
            self.thread_to_run = self.startDaemon # I'd love to be able to just pass this as an argument on start() below
            return self.start() # this will begin the thread

        # this is where the thread actually begins
        self.daemonIsRunning = True
        print '%s started' % self.name
        self.daemonStopSignal = False
        sleepStep = 0.1 # don't know how to interrupt while sleeping - so the less sleepStep, the faster StopSignal will work

        # begins the daemon in an "infinite" loop
        while self.daemonStopSignal == False and not self.exiting:
            print '%s running' % self.name
            # here, do any kind of daemon service

            delay = 0
            while self.daemonStopSignal == False and not self.exiting and delay < 1:
                #print 'Daemon sleeping'
                time.sleep(sleepStep) # delay is actually set by while, but this holds for 'sleepStep' seconds
                delay += sleepStep

        # daemon stopped, reseting everything
        self.daemonIsRunning = False
        print '%s stopped' % self.name
        #self.stop.emit(self.daemonIsRunning) # part of proposed solution
        self.emit(QtCore.SIGNAL('stopped(int)'), self.daemonIsRunning) # adapted from proposed solution
        self.emit(QtCore.SIGNAL('terminated'))



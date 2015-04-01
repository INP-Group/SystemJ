# -*- encoding: utf-8 -*-
# -*- encoding: utf-8 -*-
# -*- encoding: utf-8 -*-


#!/usr/bin/python2.5

# this is a testing module, and almost everything in here is just there to make the script work
# the relevant issues for the testing are noted in comments

from src.channels.scalarmonitor import ScalarMonitor

from PyQt4 import QtCore, QtGui
import time


class MainWindow (QtGui.QWidget):

    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)

        self.buttonDaemon = QtGui.QPushButton(self)
        self.buttonDaemon2 = QtGui.QPushButton(self)
        self.layout = QtGui.QVBoxLayout(self)
        self.layout.addWidget(self.buttonDaemon)
        self.layout.addWidget(self.buttonDaemon2)
        self.setLayout(self.layout)

        self.thread2 = Worker(name='Daemon2')
        self.thread = Worker(name='Daemon1')
        self.connect(self.thread, QtCore.SIGNAL('finished()'), self.unfreezeUi)
        self.connect(
            self.thread,
            QtCore.SIGNAL('terminated()'),
            self.unfreezeUi)
        # self.thread.stop.connect(self.stopped) # part of proposed solution
        self.connect(
            self.thread,
            QtCore.SIGNAL('stopped(int)'),
            self.stopped)  # adapted from proposed solution

        self.connect(
            self.thread2,
            QtCore.SIGNAL('finished()'),
            self.unfreezeUi2)
        self.connect(
            self.thread2,
            QtCore.SIGNAL('terminated()'),
            self.unfreezeUi2)
        # self.thread.stop.connect(self.stopped) # part of proposed solution
        self.connect(
            self.thread2,
            QtCore.SIGNAL('stopped(int)'),
            self.stopped2)  # adapted from proposed solution

        self.connect(
            self.buttonDaemon,
            QtCore.SIGNAL('clicked()'),
            self.pressDaemon)
        self.connect(
            self.buttonDaemon2,
            QtCore.SIGNAL('clicked()'),
            self.pressDaemon2)

    def unfreezeUi(self):
        self.buttonDaemon.setEnabled(True)

    def unfreezeUi2(self):
        self.buttonDaemon.setEnabled(True)

    # the problem begins below: I'm not using signals, or queue, or whatever,
    # while I believe I should for StopSignal and DaemonRunning
    def pressDaemon(self):
        # self.buttonDaemon.setEnabled(False)
        if self.thread.isDaemonRunning():
            self.thread.setDaemonStopSignal(True)
            self.buttonDaemon.setText('Daemon - run code every %s sec' % 1)
        else:
            self.thread.startDaemon()
            self.buttonDaemon.setText('Stop Daemon')
            self.buttonDaemon.setEnabled(True)

    def pressDaemon2(self):
        # self.buttonDaemon2.setEnabled(False)
        if self.thread2.isDaemonRunning():
            self.thread2.setDaemonStopSignal(True)
            self.buttonDaemon2.setText('Daemon2 - run code every %s sec' % 1)
        else:
            self.thread2.startDaemon()
            self.buttonDaemon2.setText('Stop Daemon2')
            self.buttonDaemon2.setEnabled(True)

    # part of proposed solution
    def stopped(self, val):
        print 'stopped1 ' + str(val)

    # part of proposed solution
    def stopped2(self, val):
        print 'stopped2 ' + str(val)

import time


class Testobject (QtCore.QObject):

    def __init__(self):
        super(Testobject, self).__init__()

        self.thread2 = Worker(name='Daemon2')
        self.thread = Worker(name='Daemon1')
        self.connect(self.thread, QtCore.SIGNAL('finished()'), self.unfreezeUi)
        self.connect(
            self.thread,
            QtCore.SIGNAL('terminated()'),
            self.unfreezeUi)
        # self.thread.stop.connect(self.stopped) # part of proposed solution
        self.connect(
            self.thread,
            QtCore.SIGNAL('stopped(int)'),
            self.stopped)  # adapted from proposed solution

        self.connect(
            self.thread2,
            QtCore.SIGNAL('finished()'),
            self.unfreezeUi2)
        self.connect(
            self.thread2,
            QtCore.SIGNAL('terminated()'),
            self.unfreezeUi2)
        # self.thread.stop.connect(self.stopped) # part of proposed solution
        self.connect(
            self.thread2,
            QtCore.SIGNAL('stopped(int)'),
            self.stopped2)  # adapted from proposed solution

        self.pressDaemon2()
        while True:
            time.sleep(5)
            self.pressDaemon()
            self.pressDaemon2()

        # self.connect(self.buttonDaemon, QtCore.SIGNAL('clicked()'), self.pressDaemon)
        # self.connect(self.buttonDaemon2, QtCore.SIGNAL('clicked()'), self.pressDaemon2)

    def unfreezeUi(self):
        self.buttonDaemon.setEnabled(True)

    def unfreezeUi2(self):
        self.buttonDaemon.setEnabled(True)

    # the problem begins below: I'm not using signals, or queue, or whatever,
    # while I believe I should for StopSignal and DaemonRunning
    def pressDaemon(self):
        # self.buttonDaemon.setEnabled(False)
        if self.thread.isDaemonRunning():
            self.thread.setDaemonStopSignal(True)
            # self.buttonDaemon.setText('Daemon - run code every %s sec'% 1)
        else:
            self.thread.startDaemon()
            # self.buttonDaemon.setText('Stop Daemon')
            # self.buttonDaemon.setEnabled(True)

    def pressDaemon2(self):
        # self.buttonDaemon2.setEnabled(False)
        if self.thread2.isDaemonRunning():
            self.thread2.setDaemonStopSignal(True)
            # self.buttonDaemon2.setText('Daemon2 - run code every %s sec'% 1)
        else:
            self.thread2.startDaemon()
            # self.buttonDaemon2.setText('Stop Daemon2')
            # self.buttonDaemon2.setEnabled(True)

    # part of proposed solution
    def stopped(self, val):
        print 'stopped1 ' + str(val)

    # part of proposed solution
    def stopped2(self, val):
        print 'stopped2 ' + str(val)


"""
Создать worker'а, который будет внутри содержать объект класса скалярного канала
По клику добавлять объекты в воркера
"""


class Testobject2 (QtGui.QWidget):

    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)

        self.buttonDaemon = QtGui.QPushButton(self)
        self.buttonDaemon2 = QtGui.QPushButton(self)
        self.layout = QtGui.QVBoxLayout(self)
        self.layout.addWidget(self.buttonDaemon)
        self.layout.addWidget(self.buttonDaemon2)
        self.setLayout(self.layout)

        self.thread = DaemonWorker(name='Daemon1')

        self.connect(
            self.buttonDaemon,
            QtCore.SIGNAL('clicked()'),
            self.pressDaemon)
        self.connect(
            self.buttonDaemon2,
            QtCore.SIGNAL('clicked()'),
            self.addMonitor)

    def addMonitor(self):
        self.thread.addchanel()

    # the problem begins below: I'm not using signals, or queue, or whatever,
    # while I believe I should for StopSignal and DaemonRunning
    def pressDaemon(self):

        # self.buttonDaemon.setEnabled(False)
        if self.thread.isDaemonRunning():
            self.thread.setDaemonStopSignal(True)
            self.buttonDaemon.setText('Daemon - run code every %s sec' % 1)
        else:
            self.thread.startDaemon()
            self.buttonDaemon.setText('Stop Daemon')
            self.buttonDaemon.setEnabled(True)

    # part of proposed solution
    def stopped(self, val):
        print 'stopped1 ' + str(val)


class DaemonWorker(QtCore.QThread):
    daemonIsRunning = False
    daemonStopSignal = False
    daemonCurrentDelay = 0

    def isDaemonRunning(self): return self.daemonIsRunning

    def setDaemonStopSignal(self, bool): self.daemonStopSignal = bool

    def __init__(self, parent=None, name='Daemon1'):
        QtCore.QThread.__init__(self, parent)
        self.exiting = False
        self.name = name
        self.thread_to_run = None
        self.channels = []

    def __del__(self):
        self.exiting = True
        self.thread_to_run = None
        self.wait()

    def run(self):
        if self.thread_to_run is not None:
            self.thread_to_run(mode='continue')

    def addchanel(self):
        self.channels.append(ScalarMonitor('linthermcan.ThermosM.in0'))

    # stop = QtCore.pyqtSignal(int) # part of proposed solution

    def startDaemon(self, mode='run'):
        if mode == 'run':
            # I'd love to be able to just pass this as an argument on start()
            # below
            self.thread_to_run = self.startDaemon
            return self.start()  # this will begin the thread

        # this is where the thread actually begins
        self.daemonIsRunning = True
        print '%s started' % self.name
        self.daemonStopSignal = False
        # don't know how to interrupt while sleeping - so the less sleepStep,
        # the faster StopSignal will work
        sleepStep = 0.1

        # begins the daemon in an "infinite" loop
        while self.daemonStopSignal == False and not self.exiting:
            print '%s running' % self.name
            # here, do any kind of daemon service

            delay = 0
            while self.daemonStopSignal == False and not self.exiting and delay < 1:
                # print 'Daemon sleeping'
                # delay is actually set by while, but this holds for
                # 'sleepStep' seconds
                time.sleep(sleepStep)
                delay += sleepStep

        # daemon stopped, reseting everything
        self.daemonIsRunning = False
        print '%s stopped' % self.name
        # self.stop.emit(self.daemonIsRunning) # part of proposed solution
        self.emit(
            QtCore.SIGNAL('stopped(int)'),
            self.daemonIsRunning)  # adapted from proposed solution
        self.emit(QtCore.SIGNAL('terminated'))


class Worker (QtCore.QThread):
    daemonIsRunning = False
    daemonStopSignal = False
    daemonCurrentDelay = 0

    def isDaemonRunning(self): return self.daemonIsRunning

    def setDaemonStopSignal(self, bool): self.daemonStopSignal = bool

    def __init__(self, parent=None, name='Daemon1'):
        QtCore.QThread.__init__(self, parent)
        self.exiting = False
        self.name = name
        self.thread_to_run = None

    def __del__(self):
        self.exiting = True
        self.thread_to_run = None
        self.wait()

    def run(self):
        if self.thread_to_run is not None:
            self.thread_to_run(mode='continue')

    # stop = QtCore.pyqtSignal(int) # part of proposed solution

    def startDaemon(self, mode='run'):
        if mode == 'run':
            # I'd love to be able to just pass this as an argument on start()
            # below
            self.thread_to_run = self.startDaemon
            return self.start()  # this will begin the thread

        # this is where the thread actually begins
        self.daemonIsRunning = True
        print '%s started' % self.name
        self.daemonStopSignal = False
        # don't know how to interrupt while sleeping - so the less sleepStep,
        # the faster StopSignal will work
        sleepStep = 0.1

        # begins the daemon in an "infinite" loop
        while self.daemonStopSignal == False and not self.exiting:
            print '%s running' % self.name
            # here, do any kind of daemon service

            delay = 0
            while self.daemonStopSignal == False and not self.exiting and delay < 1:
                # print 'Daemon sleeping'
                # delay is actually set by while, but this holds for
                # 'sleepStep' seconds
                time.sleep(sleepStep)
                delay += sleepStep

        # daemon stopped, reseting everything
        self.daemonIsRunning = False
        print '%s stopped' % self.name
        # self.stop.emit(self.daemonIsRunning) # part of proposed solution
        self.emit(
            QtCore.SIGNAL('stopped(int)'),
            self.daemonIsRunning)  # adapted from proposed solution
        self.emit(QtCore.SIGNAL('terminated'))

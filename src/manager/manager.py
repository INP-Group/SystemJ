# -*- encoding: utf-8 -*-

import random
import string

from PyQt4 import QtCore, QtGui
from PyQt4.QtGui import QStandardItem, QStandardItemModel

from src.manager.channellfactory import ChannelFactory
from src.manager.managerui import Ui_MainWindow as ManagerUI


class WorkerManager(ManagerUI):
    def __init__(self):
        super(WorkerManager, self).__init__()
        self.setupUi()

        self.workers = []
        width = 800
        height = 600
        self.setGeometry(0, 0, width, height)
        self.centerOnScreen()


        self.connect(self.PBAddWorker, QtCore.SIGNAL('clicked()'), self.addNewWorker)

    def centerOnScreen (self):
        '''centerOnScreen()
        Centers the window on the screen.
        '''
        resolution = QtGui.QDesktopWidget().screenGeometry()
        self.move((resolution.width() / 2) - (self.frameSize().width() / 2),
                  (resolution.height() / 2) - (self.frameSize().height() / 2))

    def getName(self, size=6, chars=string.ascii_uppercase + string.digits):
        return ''.join(random.choice(chars) for _ in range(size))

    def addNewWorker(self):
        worker = DaemonWorker(name=self.getName())
        widget = WorkerWidget(worker=worker)
        self.workers.append(worker)
        self.tWWorkers.addTab(widget, worker.getName())


class WorkerWidget(QtGui.QWidget):
    def __init__(self, parent=None, worker=None):

        QtGui.QWidget.__init__(self, parent)

        self.worker = worker

        self.gridLayout = QtGui.QGridLayout(self)
        self.gridLayout.setObjectName("gridLayout")
        self.PBAddChannel = QtGui.QPushButton(self)
        self.PBAddChannel.setObjectName("pushButton")
        self.PBAddChannel.setText("Add new random channel")
        self.gridLayout.addWidget(self.PBAddChannel, 0, 0, 1, 1)
        self.tVChannells = QtGui.QListView(self)
        self.tVChannells.setObjectName("tVChannells")
        self.gridLayout.addWidget(self.tVChannells, 0, 1, 2, 1)
        spacerItem = QtGui.QSpacerItem(20, 242, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding)
        self.gridLayout.addItem(spacerItem, 1, 0, 1, 1)

        self.connect(self.PBAddChannel, QtCore.SIGNAL('clicked()'), self.addChannell)

    def addChannell(self):
        self.worker.addchanel(random.choice(['ScalarChannel', 'NTimeChannel', 'DeltaChannel']))

        channels = self.worker.getChannels()
        model = QStandardItemModel(self.tVChannells)

        for x in channels:
            item = QStandardItem(x.getPName())
            model.appendRow(item)

        self.tVChannells.setModel(model)


class DaemonWorker(QtCore.QThread):
    '''

    10к сокетов не получается создать
    Надо сигналы одной группы собирать с помощью этого воркера
    Затем слать одно сообщение в zeromq (ну или хотя бы по M сообщений собирать в одно)
    '''

    def __init__(self, parent=None, name='Daemon1'):

        QtCore.QThread.__init__(self, parent)
        self.name = name
        self.channels = []

    def __del__(self):
        self.wait()

    def getChannels(self):
        return self.channels

    def getName(self):
        return self.name

    def addchanel(self, type="ScalarChannel", chanName="linthermcan.ThermosM.in0"):

        #todo
        type = "ScalarChannel"

        channel = ""

        if type == "ScalarChannel":
            channel = ChannelFactory.factory(type, chanName, "%s - %s" % (self.name, len(self.channels)))

        if type == "NTimeChannel":
            channel = ChannelFactory.factory(type, chanName, "%s - %s" % (self.name, len(self.channels)))
            channel.set_property("timedelta", 5.0)

        if type == "DeltaChannel":
            channel = ChannelFactory.factory(type, chanName, "%s - %s" % (self.name, len(self.channels)))
            channel.set_property("delta", .0005)

        self.channels.append(channel)

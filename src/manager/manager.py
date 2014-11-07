# -*- encoding: utf-8 -*-

from src.channels.scalarchannel import ScalarChannel

from PyQt4.QtGui import QStandardItemModel, QStandardItem
from PyQt4 import QtCore, QtGui
import string
import random
from src.manager.channellfactory import ChannelFactory
from src.manager.managerui import Ui_MainWindow as ManagerUI


class WorkerManager(ManagerUI):

    def __init__ (self):
        super(WorkerManager, self).__init__()
        self.setupUi()

        self.workers = []
        #todo
        # сделать чтобы по нормальному было по центру
        width = 800
        height = 400
        pos_x = 400
        pos_y = 300
        self.setGeometry(pos_x, pos_y, pos_x + width, pos_y + height)

        self.connect(self.PBAddWorker, QtCore.SIGNAL('clicked()'), self.addNewWorker)

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


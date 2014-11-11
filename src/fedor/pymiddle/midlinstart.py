#!/usr/bin/env python
# -*- coding: utf-8 -*-
from cothread.catools import *

old_dlopen_flags = sys.getdlopenflags()
sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)
from PyQt4.QtCore import *

sys.setdlopenflags(old_dlopen_flags)

from cdr_wrapper import *
from actl import *


class linstarter(QObject):
    def __init__(self):
        super(QObject, self).__init__()

        self.startgen = IE4()

        self.startmode_c = cxchan("ic.act.lin.startmode")
        self.startcycle_c = cxchan("ic.act.lin.startcycle")
        self.stopcycle_c = cxchan("ic.act.lin.stopcycle")
        self.startnum_c = cxchan("ic.act.lin.startnum")
        self.startcount_c = cxchan("ic.act.lin.startcount")
        self.eventcount_c = cxchan("ic.act.lin.eventcount")

        self.startgen.eventCountChanged.connect(self.updateEventCount)

        self.startmode_c.setValue(self.startgen.status["startMode"])

        self.startgen.startCountChanged.connect(self.startcount_c.setValue)
        self.startnum_c.valueChanged.connect(self.setStartNum)

        self.startcycle_c.valueChanged.connect(self.startCycle)
        self.stopcycle_c.valueChanged.connect(self.stopCycle)

        self.startgen.cycleFinished.connect(self.finishCycle)

        self.startmode_c.valueChanged.connect(self.setStartMode)
        self.startgen.startModeChanged.connect(self.updateStartMode)

        self.startmode_changing = False
        self.startmode_init = True

    def updateEventCount(self, count):
        print "new eventcount %d" % count
        self.eventcount_c.setValue(count)

    def setStartMode(self, chan, val):
        print "setstartmode"
        if self.startmode_init:
            self.startmode_init = False
            return
        if self.startmode_changing:
            self.startmode_changing = False
            return
        self.startgen.setStartMode(int(val))
        self.startmode_changing = True
        print "setstartmode %d" % val

    def updateStartMode(self, startmode):
        if self.startmode_init:
            self.startmode_init = False
        if self.startmode_changing:
            self.startmode_changing = False
            return
        self.startmode_c.setValue(startmode)
        self.startmode_changing = True
        print "updatestartmode %d" % startmode

    def setStartNum(self, chan, num):
        self.startgen.setStartNum(num)

    def startCycle(self, chan, val):
        if val == 1:
            self.startgen.startCycle()
            self.stopcycle_c.setValue(0.0)

    def stopCycle(self, chan, val):
        if val == 1:
            self.startgen.stopCycle()

    def finishCycle(self):
        self.startcycle_c.setValue(0.0)
        print "cycle finished"


app = QCoreApplication(sys.argv)

starter = linstarter()

sys.exit(app.exec_())

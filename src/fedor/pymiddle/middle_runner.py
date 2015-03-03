#!/usr/bin/env python
# -*- coding: utf-8 -*-

import DLFCN
import os

from PyQt4.QtCore import *

from actl import *
from cdr_wrapper import *

old_dlopen_flags = sys.getdlopenflags()
sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)

sys.setdlopenflags(old_dlopen_flags)


# warning! it's testing version ---- not redy for regular operation
class MiddleRunner(QObject):
    def __init__(self):
        super(QObject, self).__init__()

        self.channames = ["ic.ring.DCCT.middle_run"]
        self.runname = ["dcct_middle.py"]

        self.chans = []
        for x in self.channames:
            c = cxchan(x)
            self.chans.append(c)
            c.valueChanged.connect(self.run_middle)


    def run_middle(self, chan, val):
        ind = self.chans.index(chan)
        if val > 0:
            os.system("(%s)&" % self.runname[ind])

    def laod_set(self):
        pass


# QT init
app = QCoreApplication(sys.argv)

m_runner = MiddleRunner()


# QT main loop
sys.exit(app.exec_())

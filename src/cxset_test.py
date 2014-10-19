#!/usr/bin/env python
# -*- coding: utf-8 -*-


import sys,os
from cdr_wrapper import *
import time
from actl import *

# Special QT import, needs to make qt libs visible for Cdrlib
import DLFCN
old_dlopen_flags = sys.getdlopenflags( )
sys.setdlopenflags( old_dlopen_flags | DLFCN.RTLD_GLOBAL )
from PyQt4 import QtCore, QtGui
sys.setdlopenflags( old_dlopen_flags )





app = QtCore.QCoreApplication(sys.argv)

def cb0(chan, val):
    t1= time.time()

    for x in xrange(100000):
        chan.setValue(x)

    t2= time.time()
    print t2-t1
    app.quit()


chan = cxchan("ic.status.ic")

chan.valueChanged.connect(cb0)

app.exec_()
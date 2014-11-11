#!/usr/bin/env python
# -*- coding: utf-8 -*-

from cdr_wrapper import *

from actl import *

# Special QT import, needs to make qt libs visible for Cdrlib
import DLFCN

old_dlopen_flags = sys.getdlopenflags()
sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)
from PyQt4 import QtGui

sys.setdlopenflags(old_dlopen_flags)

# QT init
application = QtGui.QApplication(sys.argv)

# CDR
cdr = cdr()

mid1 = middlechan("ic.other._97", "ic.other._98")

# QT main loop
sys.exit(application.exec_())

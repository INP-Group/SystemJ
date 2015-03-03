#!/usr/bin/env python
# -*- coding: utf-8 -*-

import DLFCN

from PyQt4 import QtGui

from actl import *
from cdr_wrapper import *

old_dlopen_flags = sys.getdlopenflags()
sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)

sys.setdlopenflags(old_dlopen_flags)

# QT init
application = QtGui.QApplication(sys.argv)

# CDR
cdr = cdr()

mid1 = middlechan("ic.other._97", "ic.other._98")

# QT main loop
sys.exit(application.exec_())

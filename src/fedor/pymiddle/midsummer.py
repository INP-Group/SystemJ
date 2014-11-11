#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

import DLFCN

old_dlopen_flags = sys.getdlopenflags()
sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)
from PyQt4.QtCore import *

sys.setdlopenflags(old_dlopen_flags)

# from cdr_wrapper import *
from actl import *
from sdds import *


#main
if len(sys.argv) == 1:
    note = """
    summer middleware program V1.0 for CX
    by Fedor Emanov

    use example:
    midsummer settings.sdds

    settings.sdds:
    columns:
    names -  first element - middle chan, others chans to sum
    """
    print note
    sys.exit(0)

app = QCoreApplication(sys.argv)

# loading settings
settings = SDDS(0)
settings.load(sys.argv[1])
colName = settings.columnName
colData = settings.columnData
names = colData[colName.index("names")][0]
midname = names[0]
srcnames = names[1:len(names) - 1]

sum = middleSummer(srcnames, midname)

runctl = middleRunCtl(app, "ic.linmag.middlerun")

print "summer middleware program V1.0 for CX"

sys.exit(app.exec_())
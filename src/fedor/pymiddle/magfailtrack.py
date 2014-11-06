#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

import DLFCN
old_dlopen_flags = sys.getdlopenflags()
sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)
from PyQt4 import QtGui
from PyQt4.QtCore import *
sys.setdlopenflags(old_dlopen_flags)

#from cdr_wrapper import *
from actl import *
from sdds import *


srcnames_ex = {
'Iset'  : '',
'Imeas' : '',
'Umeas' : '',
'block' : '',
'swc'   : ''
}

settings_ex = {
'Imax': 0.0,
'Imin': 0.0,
'Umax': 0.0,
'Itol': 0.0,
}


#main
if len(sys.argv) == 1:
    note = """
    middleware program V1.0 for CX
    by Fedor Emanov

    use example:

    """
    print note
    sys.exit(0)


app = QCoreApplication(sys.argv)

# loading settings
settings = SDDS(0)
settings.load(sys.argv[1])
colName = settings.columnName
colData = settings.columnData

if

names = colData[colName.index("names")][0]
midname = names[0]
srcnames = names[1:len(names)-1]

sum = middleSummer(srcnames, midname)

runctl = middleRunCtl(app, "ic.linmag.middlerun")

print "summer middleware program V1.0 for CX"

sys.exit(app.exec_())
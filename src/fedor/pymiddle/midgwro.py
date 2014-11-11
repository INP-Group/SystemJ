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


# class here is not needed
class middle_gw_ro():
    def __init__(self, settings_file):
        settings = SDDS(0)
        settings.load(settings_file)

        colName = settings.columnName
        colData = settings.columnData

        srcnames = colData[colName.index("srcname")][0]
        midnames = colData[colName.index("midname")][0]

        self.mchans = []
        for ind in range(len(srcnames)):
            self.mchans.append(middleChanRO(srcnames[ind], midnames[ind]))


if len(sys.argv) == 1:
    note = """
    read only middleware program V1.0 for CX
    by Fedor Emanov

    use example:
    midgw ro settings.sdds

    settings.sdds:
    columns:
    srcname - name of src chansels
    midname - mane of middleware chanels
    """
    print note
    sys.exit(0)

app = QCoreApplication(sys.argv)

mid = middle_gw_ro(sys.argv[1])

runctl = middleRunCtl(app, "ic.linmag.middlerun")

print "middleware polinomial translator runing"
print "serving channels: %d" % len(mid.mchans)

sys.exit(app.exec_())
#!/usr/bin/env python
# -*- coding: utf-8 -*-

import DLFCN
import sys

import numpy as np
from PyQt4.QtCore import *

from actl import *
from sdds import *

old_dlopen_flags = sys.getdlopenflags()
sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)

sys.setdlopenflags(old_dlopen_flags)



class middle_gw():
    def __init__(self, settings_file):
        settings = SDDS(0)
        settings.load(settings_file)

        colName = settings.columnName
        colData = settings.columnData

        srcnames = colData[colName.index("srcname")][0]
        midnames = colData[colName.index("midname")][0]
        srcmin = colData[colName.index("smin")][0]
        srcmax = colData[colName.index("smax")][0]

        self.mchans = []
        self.srclims = np.zeros((len(srcnames), 2))
        for ind in range(len(srcnames)):
            self.srclims[ind][0] = srcmin[ind]
            self.srclims[ind][1] = srcmax[ind]
            self.mchans.append(middleChan(srcnames[ind], midnames[ind], self.srclims[ind]))


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

    smin - low limit
    smax - high limit
        limits are fo src channel
    """
    print note
    sys.exit(0)

app = QCoreApplication(sys.argv)

mid = middle_gw(sys.argv[1])

runctl = middleRunCtl(app, "ic.linmag.middlerun")

print "middleware polinomial translator runing"
print "serving channels: %d" % len(mid.mchans)

sys.exit(app.exec_())

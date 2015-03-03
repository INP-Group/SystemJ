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




class middle_gw_rw():
    def __init__(self, settings_file, s2mname=None, m2sname=None):
        # loading settings
        settings = SDDS(0)
        settings.load(settings_file)

        # polinoms order
        if s2mname is None:
            self.order = settings.parameterData[settings.parameterName.index("order")][0]
        else:
            self.order = len(s2mname) - 1

        colName = settings.columnName
        colData = settings.columnData

        srcnames = colData[colName.index("srcname")][0]
        midnames = colData[colName.index("midname")][0]
        srcmin = colData[colName.index("smin")][0]
        srcmax = colData[colName.index("smax")][0]

        self.mchans = []
        self.s2m = np.zeros((len(srcnames), self.order + 1))
        self.m2s = np.zeros((len(srcnames), self.order + 1))
        self.srclims = np.zeros((len(srcnames), 2))
        for ind in range(len(srcnames)):
            if s2mname is None:
                for y in range(self.order + 1):
                    self.s2m[ind][y] = colData[colName.index("s2m%d" % y)][0][ind]
                    self.m2s[ind][y] = colData[colName.index("m2s%d" % y)][0][ind]
            else:
                for y in range(self.order + 1):
                    if s2mname[y] == '':
                        self.s2m[ind][y] = 0.0
                    else:
                        self.s2m[ind][y] = colData[colName.index(s2mname[y])][0][ind]
                    if m2sname[y] == '':
                        self.m2s[ind][y] = 0.0
                    else:
                        self.m2s[ind][y] = colData[colName.index(m2sname[y])][0][ind]
            self.srclims[ind][0] = srcmin[ind]
            self.srclims[ind][1] = srcmax[ind]
            self.mchans.append(
                middleChanPoly(srcnames[ind], midnames[ind], self.srclims[ind], self.s2m[ind], self.m2s[ind]))

#main

if len(sys.argv) == 1:
    note = """
    read-write polinomial middleware program V1.0 for CX
    by Fedor Emanov

    use example:
    midpoly.py settings.sdds

    settings.sdds:
    parameter:
    order - order of polinom
    columns:
    srcname - name of src chansels
    midname - mane of middleware chanels
    s2m%d, m2s%d - name of polinom koefs
    smin - low limit
    smax - high limit
        limits are fo src channel

    use example:
    midpolyrw.py settings.sdds ,,A,,B ,,C,,D
    ,,A,,B ,,C,,D - coma separated names of coefficient columns
    must have the same length. Length define order,
    empty column name means zero coefficients

    """
    print note
    sys.exit(0)

app = QCoreApplication(sys.argv)

if len(sys.argv) == 2:
    mid = middle_gw_rw(sys.argv[1])
else:
    s2mname = sys.argv[2].split(",")
    m2sname = sys.argv[2].split(",")
    mid = middle_gw_rw(sys.argv[1], s2mname, m2sname)

runctl = middleRunCtl(app, "ic.linmag.middlerun")

print "middleware polinomial translator runing"
print "serving channels: %d" % len(mid.mchans)

sys.exit(app.exec_())

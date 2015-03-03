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




class middle_gw_ro():
    def __init__(self, settings_file, s2mname=None):
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

        self.mchans = []
        self.s2m = np.zeros((len(srcnames), self.order + 1))
        for ind in range(len(srcnames)):
            if s2mname is None:
                for y in range(self.order + 1):
                    self.s2m[ind][y] = colData[colName.index("s2m%d" % y)][0][ind]
            else:
                for y in range(self.order + 1):
                    if s2mname[y] == '':
                        self.s2m[ind][y] = 0.0
                    else:
                        self.s2m[ind][y] = colData[colName.index(s2mname[y])][0][ind]

            self.mchans.append(middleChanPolyRO(srcnames[ind], midnames[ind], self.s2m[ind]))

#main

if len(sys.argv) == 1:
    note = """
    read only polinomial middleware program V1.0 for CX
    by Fedor Emanov

    use example:
    midpolyro settings.sdds

    settings.sdds:
    parameter:
    order - order of polinom
    columns:
    srcname - name of src chansels
    midname - mane of middleware chanels
    s2m%d - name of polinom koefs

    use example:
    midpolyro.py settings.sdds ,,A,,B
    ,,A,,B - coma separated names of coefficient columns
    Length define order, empty column name means zero coefficients

    """
    print note
    sys.exit(0)

app = QCoreApplication(sys.argv)

if len(sys.argv) == 2:
    mid = middle_gw_ro(sys.argv[1])
else:
    s2mname = sys.argv[2].split(",")
    mid = middle_gw_ro(sys.argv[1], s2mname)

runctl = middleRunCtl(app, "ic.linmag.middlerun")

print "middleware polinomial translator runing"
print "serving channels: %d" % len(mid.mchans)

sys.exit(app.exec_())

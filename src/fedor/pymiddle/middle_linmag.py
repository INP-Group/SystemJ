#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import cothread
from cothread.catools import *

import DLFCN
old_dlopen_flags = sys.getdlopenflags()
sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)
from PyQt4 import QtGui
sys.setdlopenflags(old_dlopen_flags)

#from cdr_wrapper import *
from actl import *
from sdds import *

import numpy as np


class middle_gw():
    def __init__(self, settings_file):
        self.settings = SDDS(0)
        self.settings.load(settings_file)

        self.srcnames = self.settings.columnData[self.settings.columnName.index("srcnames")][0]
        self.midnames = self.settings.columnData[self.settings.columnName.index("midnames")][0]

        # max currents
        self.srcmax = self.settings.columnData[self.settings.columnName.index("srcmax")][0]
        self.srcmax = self.settings.columnData[self.settings.columnName.index("srcmin")][0]

        self.s2m0 = self.settings.columnData[self.settings.columnName.index("kig0")][0]
        self.s2m1 = self.settings.columnData[self.settings.columnName.index("kig1")][0]
        self.s2m2 = self.settings.columnData[self.settings.columnName.index("kig2")][0]
        self.s2m3 = self.settings.columnData[self.settings.columnName.index("kig3")][0]
        self.s2m4 = self.settings.columnData[self.settings.columnName.index("kig4")][0]
        self.s2m5 = self.settings.columnData[self.settings.columnName.index("kig5")][0]

        self.m2s0 = self.settings.columnData[self.settings.columnName.index("kgi0")][0]
        self.m2s1 = self.settings.columnData[self.settings.columnName.index("kgi1")][0]
        self.m2s2 = self.settings.columnData[self.settings.columnName.index("kgi2")][0]
        self.m2s3 = self.settings.columnData[self.settings.columnName.index("kgi3")][0]
        self.m2s4 = self.settings.columnData[self.settings.columnName.index("kgi4")][0]
        self.m2s5 = self.settings.columnData[self.settings.columnName.index("kgi5")][0]


        self.mchans = []
        for x in self.srcnames:
            ind = self.srcnames.index(x)
            self.mchans.append(middleChanPoly(x, self.midnames[ind]))


class middle_gw_ro():
    def __init__(self, settings_file):
        self.settings = SDDS(0)
        self.settings.load(settings_file)

        self.srcnames = self.settings.columnData[self.settings.columnName.index("srcname")][0]
        self.midnames = self.settings.columnData[self.settings.columnName.index("midname")][0]

        self.s2m0 = self.settings.columnData[self.settings.columnName.index("kig0")][0]
        self.s2m1 = self.settings.columnData[self.settings.columnName.index("kig1")][0]
        self.s2m2 = self.settings.columnData[self.settings.columnName.index("kig2")][0]
        self.s2m3 = self.settings.columnData[self.settings.columnName.index("kig3")][0]
        self.s2m4 = self.settings.columnData[self.settings.columnName.index("kig4")][0]
        self.s2m5 = self.settings.columnData[self.settings.columnName.index("kig5")][0]

        self.s2ma = []  # s2m array for all s2m arrays
        self.mchans = []
        for x in self.srcnames:
            ind = self.srcnames.index(x)
            self.s2ma.append(np.array([self.s2m0[ind], self.s2m1[ind], self.s2m2[ind], self.s2m3[ind], self.s2m4[ind], self.s2m5[ind]]))
            self.mchans.append(middleChanPolyRO(x, self.midnames[ind], self.s2ma[ind]))


app = cothread.iqt()


mid = middle_gw_ro(sys.argv[1])


cothread.WaitForQuit()

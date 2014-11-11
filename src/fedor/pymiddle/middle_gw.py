#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import DLFCN

import cothread
from cothread.catools import *


old_dlopen_flags = sys.getdlopenflags()
sys.setdlopenflags(old_dlopen_flags | DLFCN.RTLD_GLOBAL)

sys.setdlopenflags(old_dlopen_flags)

# from cdr_wrapper import *
from actl import *
from sdds import *


class middle_gw():
    def __init__(self, settings_file):
        self.settings = SDDS(0)
        self.settings.load(settings_file)

        self.pvs = self.settings.columnData[self.settings.columnName.index("pvs")][0]
        self.cxnames = self.settings.columnData[self.settings.columnName.index("cxnames")][0]

        self.mchans = []
        for x in self.pvs:
            ind = self.pvs.index(x)
            self.mchans.append(middleChanEpics(x, self.cxnames[ind]))


app = cothread.iqt()

mid = middle_gw(sys.argv[1])

cothread.WaitForQuit()

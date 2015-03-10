# -*- encoding: utf-8 -*-
# CX middleware run control class by Fedor Emanov
# CX middleware is modular set of programs used for
# accelerator-level control

from actl.cxchan import *


class middleRunCtl:
    def __init__(self, app, runChanName):
        self.app = app
        self.runChan = cxchan(runChanName)
        self.runChan.valueChanged.connect(self.runChanged)
        self.runChan.setValue(1.0)
        self.app.aboutToQuit.connect(self.finisher)

    def runChanged(self, chan, run):
        if run == 0:
            self.app.quit()

    def finisher(self):
        self.runChan.setValue(0.0)

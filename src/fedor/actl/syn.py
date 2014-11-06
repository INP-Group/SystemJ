# VEPP-5 injection complex syn classes
# by Fedor Emanov


from actl.cxchan import *
from cothread.catools import *
from PyQt4.QtCore import *


statusIE4 = {
    "clockSrc"  : 0, # bit 1-2
    "startSrc"  : 0, # bit 3-4
    "startMode" : 0, # bit 5
    "LAMmask"   : 0, # bit 6
    "cycleStop" : 0, # bit 7
}


# control pvs for IE4
# V5:SYN:CountC - linac start counter write channel uint16
# V5:SYN:CountM - linac start counter monitor channel
# V5:SYN:Status00C.B4 - off/on counter
# V5:SYN:BPS:SwitchC
# V5:SYN:StartC.PROC - write 1 to allow counter run
# V5:SYN:StopC.PROC -  write 1 to forbid counter run
# V5:SYN:Status00M -  uint16 monitor status register
# V5:SYN:EventM - int32 cycle counter

class IE4(QObject):

    # sygnals for user
    cycleStarted = pyqtSignal()
    cycleStoped = pyqtSignal()
    cycleFinished = pyqtSignal()

    eventCountChanged = pyqtSignal(int)

    #signals for inside use
    statusChanged = pyqtSignal()
    startCountChanged = pyqtSignal(int)
    startModeChanged = pyqtSignal(int)

    startSig = pyqtSignal()
    stopSig = pyqtSignal()

    def __init__(self):
        super(QObject, self).__init__()

        pvs = "V5:SYN:Status00C.B4", "V5:SYN:StopC.PROC", "V5:SYN:StartC.PROC",


        # loading initial state
        self.statusVal = caget("V5:SYN:Status00M")
        self.status = self.parseStatus(self.statusVal)
        self.eventCount = caget("V5:SYN:EventM")
        self.startCount = caget("V5:SYN:CountM")

        # starting monitors
        self.eventm_pv = camonitor("V5:SYN:EventM", self.eventCB)
        self.statusm_pv = camonitor("V5:SYN:Status00M", self.statusCB)
        self.countm_pv = camonitor("V5:SYN:CountM", self.countCB)

        self.startNum = 0

    def parseStatus(self, statusValAny):
        statusVal = int(statusValAny)
        status = statusIE4.copy()
        status["clockSrc"] = (statusVal & 3)
        status["startSrc"] = ((statusVal >> 2) & 3)
        status["startMode"] = ((statusVal >> 4) & 1)
        status["LAMmask"] = ((statusVal >> 5) & 1)
        status["cycleStop"] = ((statusVal >> 6) & 1)
        return status

    # callback for
    def statusCB(self, statusVal):
        status = self.parseStatus(statusVal)
        if statusVal != self.statusVal:
            self.statusVal = statusVal
            self.statusChanged.emit()
        if status["startMode"] != self.status["startMode"]:
            self.startModeChanged.emit(status["startMode"])
        if status["cycleStop"] != self.status["cycleStop"]:
            self.cycleStoped.emit()
        self.status = status

    def eventCB(self, value):
        self.eventCountChanged.emit()
        if self.eventCount != value:
            self.eventCount = value
            self.cycleFinished.emit()

    def countCB(self, count):
        self.startCount = count
        self.startCountChanged.emit(count)

    def setStartNum(self, num):
        self.startNum = num

    def setStartMode(self, startMode):
        caput("V5:SYN:Status00C.B4", startMode)

    def startCycle(self):
        caput("V5:SYN:CountC", self.startNum)
        if self.startCount == self.startNum and self.status["startMode"] == 1:
            self.runCycle()
            return
        if self.startCount != self.startNum:
            self.startCountChanged.connect(self.validateStart)
        if self.status["startMode"] != 1:
            caput("V5:SYN:Status00C.B4", 1)
            self.statusChanged.connect(self.validateStart)



    @staticmethod
    def stopCycle():
        caput("V5:SYN:StopC.PROC", 1)

    @staticmethod
    def runCycle():
        caput("V5:SYN:StartC.PROC", 1)

    # v=none - for compatibility. ignored.
    def validateStart(self, v=None):
        if self.startNum == self.startCount and self.status["startMode"] == 1:
            try:
                self.startCountChanged.disconnect(self.validateStart)
            except:
                pass
            try:
                self.statusChanged.disconnect(self.validateStart)
            except:
                pass
            self.runCycle()


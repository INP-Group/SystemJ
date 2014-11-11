import numpy as np

from PyQt4.QtCore import *


# adc200 parameters names and numders (as if Bolkhov's driver)
# some mo parameters are not needed by program
# SHOT=0, ISTART=1, WAITTIME=2, STOP=3,
adc200par = dict(PTSOFS=4, NUMPTS=5, TIMING=6, FRQDIV=7,
                 RANGE1=8, RANGE2=9, ZERO1=10, ZERO2=11)

freqdev = [1, 2, 3, 4]

clock = [200.0, 195.0, 178.4359]

scale = [0.256, 0.512, 1.024, 2.048]

quant = [0.002, 0.004, 0.008, 0.016]

ADC200_MAX_NUMPTS = 128000


class adc200(QObject):
    # signals
    parChanged = pyqtSignal(str, int)
    newData = pyqtSignal()
    newRef = pyqtSignal()
    newAvg = pyqtSignal()
    refDif = pyqtSignal()

    def __init__(self, name):
        super(QObject, self).__init__()

        self.cdr = cdr()

        self.parvals = adc200par.copy()
        for k in self.parvals:
            self.parvals[k] = 0
        self.prev_parvals = self.parvals.copy()

        self.data = np.zeros(0, dtype=np.uint8)
        self.udata = np.zeros(0, dtype=np.float32)
        self.tdata = np.zeros(0, dtype=np.float)

        # averaging: class can calculate average data
        self.averaging = False
        self.anum = 0
        self.adata = None
        self.acount = 0

        # background substruction
        self.ground_sub = False
        self.ground = 0
        self.ground_calibrating = False
        self.ground_is_vector = False
        self.ground_anum = 1
        self.ground_acount = 0

        # reference signal data - will appear when needed
        self.refdata = None

        self.timestep = 5e-09
        self.handle = self.cdr.RegisterSimpleBigc(name, ADC200_MAX_NUMPTS, self.cb)

    def cb(self, handle, params):
        self.get_scope_params()
        self.cdr.GetSimpleBigcData(self.handle, 0, 2 * self.parvals['NUMPTS'], self.data.ctypes.data)
        self.data_convert()
        if self.ground_sub:
            self.udata -= self.ground

        if self.ground_calibrating:
            print 'ground: cycle'
            if self.ground_is_vector:
                self.ground += self.udata
            else:
                self.ground += np.mean(self.udata, axis=1)
            self.ground_acount -= 1
            if self.ground_acount < 1:
                self.ground_endcalibrate()

        if self.averaging:
            print 'acycle', self.acount
            if self.acount == 0:
                self.adata = self.udata.copy()
            else:
                self.adata += self.udata
            self.acount += 1
            if self.acount == self.anum:
                self.adata /= self.anum
                self.acount = 0
                self.newAvg.emit()

        self.newData.emit()
        return 0

    def get_scope_params(self):
        timedata_changed = False
        for k in adc200par:
            self.parvals[k] = self.cdr.GetSimpleBigcParam(self.handle, adc200par[k])
            if self.prev_parvals[k] != self.parvals[k]:
                self.prev_parvals[k] = self.parvals[k]
                if k == 'NUMPTS':
                    self.data.resize((2, self.parvals['NUMPTS']), refcheck=False)
                    self.udata.resize((2, self.parvals['NUMPTS']), refcheck=False)
                    self.ground_reset()
                    timedata_changed = True
                if k == 'TIMING' or k == 'FRQDIV':
                    timedata_changed = True
                self.parChanged.emit(k, self.parvals[k])
        if timedata_changed:
            self.timedata()

    def set_scope_param(self, parname, val):
        self.cdr.SetSimpleBigcParam(self.handle, adc200par[str(parname)], val)

    def timedata(self):
        self.tdata = np.arange(0, self.parvals['NUMPTS'] * self.timestep, self.timestep)
        self.tdata *= 1.0e9

    def data_convert(self):
        self.udata[0] = (self.data[0].astype(float) + self.parvals['ZERO1'] - 256.0) * quant[self.parvals['RANGE1']]
        self.udata[1] = (self.data[1].astype(float) + self.parvals['ZERO2'] - 256.0) * quant[self.parvals['RANGE2']]

    def ground_calibrate(self, is_vector=True, anum=1):
        print 'calibrating ground'
        self.ground_is_vector = is_vector
        self.ground_anum = anum
        self.ground_acount = anum
        self.ground_calibrating = True
        if self.ground_is_vector:
            np.zeros(shape=(2, self.parvals['NUMPTS']), dtype=np.float32)

    def ground_endcalibrate(self):
        print "ground calibration done"
        self.ground_calibrating = False
        self.ground /= self.ground_anum
        self.ground_sub = True

    def ground_reset(self):
        self.ground_sub = False
        self.ground = 0
        self.ground_calibrating = False
        self.ground_is_vector = False

    def reference_save(self):
        self.refdata = self.udata.copy()
        self.newRef.emit()

    def reference_reset(self):
        self.refdata = None
        self.newRef.emit()

    def average(self, anum):
        if anum > 1:
            self.averaging = True
            self.anum = anum
            self.adata = self.udata.copy()
            self.acount = 1
        else:
            self.averaging = False
            self.adata = None


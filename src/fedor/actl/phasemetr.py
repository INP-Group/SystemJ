#!/usr/bin/env python
# -*- coding: utf-8 -*-

import numpy as np
import math

from cdr_wrapper import *
import scipy as sp
import actl
from actl.cxchan import *

# reference voltage measurement calibrations
uref_c = {
    'Ka': 2.0021,
    'Kb': 4.9866,
    'U0': 0.778,
    'U1': 0.0963
}

# channels:
# 0 - blok1, chan1
# 1 - blok1, chan2
# 2 - blok2, chan1
# 3 - blok2, chan2
#
bafi_chans = 4

uph0_c_old = {
    'k1': [0.7125, 0.7125, 0.689, 0.718],
    'k2': [0.702, 0.702, 0.726, 0.711],
    'a': [-0.121, -0.125, -0.136, -0.133],
    'b': [0.119, 0.123, 0.135, 0.132]
}

uph0_c = np.array([-0.0001545, -0.0001125, -0.005721, 0.000213])

urf_c = {
    'ka': [12.4441, 12.1751, 10.7206, 10.7216],
    'kb': [514.7942, 579.4755, 25.5438, 21.5801],
    'u0': [0.2546, 0.2671, 0.3753, 0.3693],
    'u1': [0.0677, 0.0607, 0.0554, 0.0631]
}

comm_c = [0.5074, 0.5065, 0.5089, 0.5118]

s21_c = {
    'rs': [0.02225, 0.03125, 0.02705, 0.02957],
    'zl': [1.76626, 1.9146, 1.8552, 1.93608],
    'zp': [0.73959, 0.8138, 0.78018, 0.76168],
    'bc': [0.348, 0.348, 0.348, 0.348],
    'gu': [0.427, 0.427, 0.429, 0.421],
    'kadp': [0.41954, 0.40733, 0.34885, 0.36248]
}

ph_data_c = {
    'k1': [0.7125, 0.7125, 0.689, 0.718],
    'k2': [0.702, 0.702, 0.726, 0.711],
    'k3': [0.104, 0.104, 0.1112, 0.1131],
    'k4': [0.102, 0.102, 0.1104, 0.1122],
    'k5': [1.163, 1.206, 1.22, 1.18],
    'ufi0': [-1e-3, -1e-3, -1e-3, -1e-3]
}


class phasemetr_single():
    def __init__(self, data_cb, shift_cb, uref_cb, commut_cb):
        self.cdr = cdr()
        # callbacks
        self.data_cb = data_cb
        self.shift_cb = shift_cb
        self.uref_cb = uref_cb
        self.commut_cb = commut_cb


class phasemetr():
    def __init__(self, data_cb, shift_cb, uref_cb, commut_cb):
        self.cdr = cdr()
        # callbacks
        self.data_cb = data_cb
        self.shift_cb = shift_cb
        self.uref_cb = uref_cb
        self.commut_cb = commut_cb

        # reference voltage measurement ADC
        self.uref_chan = cxchan('sukhpanel.s-m.ch0')
        self.uref_chan.valueChanged.connect(self.cb)

        self.uref = 0  # RF reference voltage
        self.uout = 0  # adc for RF reference voltage

        # phase shifters
        self.ph_shs = [None, None, None, None]
        self.ph_shs[0] = actl.PhaseShifterB('sukhpanel.s-m.dac.set0', 0, self.phase_cb)
        self.ph_shs[1] = actl.PhaseShifterB('sukhpanel.s-m.dac.set1', 1, self.phase_cb)
        self.ph_shs[2] = actl.PhaseShifterB('sukhpanel.s-m.dac.set2', 2, self.phase_cb)
        self.ph_shs[3] = actl.PhaseShifterB('sukhpanel.s-m.dac.set3', 3, self.phase_cb)

        self.adc200_0 = actl.adc200('sukhpanel.adc-0.adc')
        self.adc200_1 = actl.adc200('sukhpanel.adc-1.adc')
        self.adc200_0.newData.connect(self.adc0_cb)
        self.adc200_1.newData.connect(self.adc1_cb)

        self.comm = [0, 0]
        cnames = ['sukhpanel.adc-0.comm0', 'sukhpanel.adc-0.comm1']
        self.comm[0] = actl.sim_ctl(cnames, self.comm_cb)
        cnames = ['sukhpanel.adc-1.comm0', 'sukhpanel.adc-1.comm1']
        self.comm[1] = actl.sim_ctl(cnames, self.comm_cb)

        self.uph0 = np.zeros(bafi_chans)

        # voltage at BAFI exit (before commutators)
        self.umeas = [None, None, None, None]
        self.tdata = [None, None]
        self.urf = [None, None]
        self.ua = [None, None]
        self.phi = [np.array(0.0), np.array(0)]

    # @profile
    def phase_cb(self, sender):
        self.shift_cb(sender)

    #@profile
    def cb(self):
        self.uout = self.uref_chan.val
        self.count_uref()
        self.count_uph0()
        self.uref_cb()

    #@profile
    def adc0_cb(self):
        if self.comm[0].val < 2:
            chan = int(self.comm[0].val)
        else:
            return

        self.umeas[0] = self.count_umeas(0, self.adc200_0.udata[0])
        self.umeas[1] = self.count_umeas(1, self.adc200_0.udata[1])
        self.tdata[0] = self.adc200_0.tdata
        # adc200 returns numpy array - valid for vector operations
        self.urf[0] = self.count_urf(chan, self.umeas[1])

        s21 = self.s21_rf(chan)

        self.ua[0] = self.u_amplitude(self.urf[0], s21)
        self.phi[0] = self.phase_rf(chan, self.ua[0], self.umeas[0])

        self.data_cb()

    #@profile
    def adc1_cb(self):
        if self.comm[1].val < 2:
            chan = 2 + int(self.comm[1].val)
        else:
            return

        self.umeas[2] = self.count_umeas(0, self.adc200_1.udata[0])
        self.umeas[3] = self.count_umeas(1, self.adc200_1.udata[1])
        self.tdata[1] = self.adc200_1.tdata
        # adc200 returns numpy array - valid for vector operations
        self.urf[1] = self.count_urf(chan, self.umeas[3])

        s21 = self.s21_rf(chan)
        self.ua[1] = self.u_amplitude(self.urf[1], s21)
        self.phi[1] = self.phase_rf(chan, self.ua[0], self.umeas[2])

        self.data_cb()

    #@profile
    def comm_cb(self):
        self.commut_cb()

    #@profile
    def count_uref(self):
        """ call when uout measured  """
        m_uout = math.fabs(self.uout)
        if m_uout < 0.01:
            self.uref = 0.0
            return
        self.uref = m_uout * uref_c['Ka'] + uref_c['U0'] + uref_c['U1'] * math.log(uref_c['Kb'] * m_uout + 1.0)

    #@profile
    def count_uph0(self):
        self.uph0 = uph0_c * self.uref

    #@profile
    def count_umeas(self, comm_chan, data):
        """data - numpy array with scope data"""
        return (data / comm_c[comm_chan])

    #@profile
    def s21_rf(self, chan):
        """
        phase shifter voltage pass coefficient calculator
        """
        uc = self.ph_shs[chan].uc

        rs = s21_c['rs'][chan]
        zl = s21_c['zl'][chan]
        zp = s21_c['zp'][chan]
        bc = s21_c['bc'][chan]
        gu = s21_c['gu'][chan]
        kadp = s21_c['kadp'][chan]

        zp2 = zp * zp
        znam_p = bc * math.pow(uc, gu)
        znam11 = ( znam_p - zl ) * ( znam_p - zl )
        znam1 = zp2 * ( znam11 + (rs + 1.0) * (rs + 1.0) - 1.0)
        znam2 = (znam_p + zp - zl) * (znam_p + zp - zl)
        znam = znam1 + znam2 + rs * rs
        s21 = ( 1.0 - (4.0 * rs * zp2) / znam ) * kadp

        return (s21)

    #@profile
    def count_ubafi(self, chan):
        pass

    @staticmethod
    def count_urf(chan, uam):
        """ call when need to process new adc200 data """
        urf = uam * urf_c['ka'][chan] + urf_c['u0'][chan] \
              + urf_c['u1'][chan] * np.log((urf_c['kb'][chan] * uam + 1.0))
        return urf

    #BAFI amplitude exit calculation

    @staticmethod
    def u_amplitude(u_rf, s21):
        ua = u_rf * np.sqrt(s21)
        return (ua)

    #@profile
    def phase_rf(self, chan, ua, v):
        k1 = ph_data_c['k1'][chan]
        k2 = ph_data_c['k2'][chan]
        k3 = ph_data_c['k3'][chan]
        k4 = ph_data_c['k4'][chan]
        k5 = ph_data_c['k5'][chan]
        ufi0 = ph_data_c['ufi0'][chan]

        k1_2 = k1 ** 2
        k2_2 = k2 ** 2
        k3_2 = k3 ** 2
        k4_2 = k4 ** 2
        k5_2 = k5 ** 2
        uref_2 = self.uref ** 2

        uref = self.uref
        v0 = self.uph0[chan] - ufi0

        size = list(ua.shape)[0]

        phi = np.zeros(size, dtype=np.float)

        for i in range(size):
            if ua[i] == 0:
                phi[i] = 0
            else:
                ua_2 = ua[i] ** 2
                dv = v[i] - v0
                dv_2 = dv ** 2
                chisl = k4 * np.sqrt((ua_2 + uref_2) * (k3_2 + k4_2) - dv_2 / k5_2) - dv * k3 / k5
                y = chisl / (k3_2 + k4_2)
                ar = (ua_2 * k1_2 + uref_2 * k2_2 - y * y) / (2.0 * k1 * k2 * ua[i] * uref)
                if ar < -1:
                    f = - sp.pi / 2.0
                else:
                    if ar > 1:
                        f = sp.pi / 2.0
                    else:
                        f = np.arcsin(ar)
                phi[i] = f * 180.0 / math.pi
        return (phi)




















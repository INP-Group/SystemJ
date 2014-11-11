import math
from actl.cxchan import *

#
srcnames_ex = {
    'Iset': '',
    'Imeas': '',
    'Umeas': '',
    'block': '',
    'swc': ''
}

settings_ex = {
    'Imax': 0.0,
    'Imin': 0.0,
    'Umax': 0.0,
    'Itol': 0.0,
}


class FailTrackerMag(object):
    def __init__(self, srcnames, failname, settings):
        self.settings = settings

        self.failchan = cxchan(failname)

        self.srcchans = {}
        self.srcvals = {}
        for k, v in srcnames:
            chan = cxchan(v)
            chan.valueChanged.connect(self.src_cb)
            self.srcchans[chan] = k
            self.srcvals[k] = 0.0

    def src_cb(self, chan, value):
        k = self.srcchans[chan]
        self.srcvals[k] = value
        if self.fail_check():
            self.failchan.setValue(1)
        else:
            self.failchan.setValue(0)

    def fail_check(self):
        """
        False - working properly
        True - failure
        """
        # may be that's very bad tests organization.
        # need to check drivers convention for blocking ang switches

        dItest = math.fabs(self.srcvals['Iset'] - self.srcvals['Imeas']) > self.settings['Itol']
        Imaxtest = math.fabs(self.srcvals['Iset'] - self.settings['Imax']) < self.settings['Itol']

        # optional tests
        Umaxtest = False
        if 'Umeas' in self.srcvals:
            Umaxtest = self.srcvals['Umeas'] >= self.settings['Umax']

        blocktest = False
        if 'block' in self.srcvals:
            blocktest = self.srcvals['block'] > 0

        swctest = False
        if 'swc' in self.srcvals:
            swctest = self.srcvals['swc'] == 0

        res = dItest or Imaxtest or Umaxtest or blocktest or swctest

        return res
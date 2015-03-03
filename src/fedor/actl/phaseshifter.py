import math

import actl

# control voltage coeffs
uc_c = [6.04, 5.999, 5.987, 5.88]
# valid phase intervals
ucq = 305.2e-06
uc_low = [1085.0 * ucq, 1091.0 * ucq, 2190.0 * ucq, 2229.0 * ucq]
uc_high = [21699.0 * ucq, 21841.0 * ucq, 20796.0 * ucq, 21732.0 * ucq]

phase_c = {
    'rs': [0.2, 0.28, 0.13, 0.3],
    'zl': [1.783, 1.767, 1.743, 1.691],
    'zp': [0.863, 0.972, 0.824, 1.081],
    'b': [0.341, 0.311, 0.295, 0.234],
    'g': [0.423, 0.437, 0.453, 0.493],
    'fi0': [-46.123, -35.816, -42.214, -18.393]
}


class PhaseShifterB():
    """
    BAFI phase shifter class
    """

    def __init__(self, name, chan, change_cb):
        self.cb = change_cb
        self.name = name
        self.chan = chan

        self.cxchan = actl.cxchan(name)
        self.cxchan.valueChanged.connect(self.cb_in)

        self.uc = None
        self.phi = None

    def cb_in(self):
        self.uc = uc_c[self.chan] * self.cxchan.val
        self.phi = self.count_phase(self.uc)
        self.cb(self)

    def set_phase(self, phi):
        self.phi = phi
        self.uc = self.count_uc(phi)
        self.cxchan.cset(self.uc / uc_c[self.chan])

    def count_phase(self, uc):
        chan = self.chan
        b = phase_c['b'][chan]
        g = phase_c['g'][chan]
        zl = phase_c['zl'][chan]
        zu = b * math.pow(uc, g) - zl
        zu2 = zu ** 2

        zp = phase_c['zp'][chan]
        zp2 = math.pow(zp, 2.0)
        zp4 = zp2 ** 2
        a = zp2 - 1.0

        rs = phase_c['rs'][chan]
        rs2 = rs ** 2

        phi = math.atan((-2.0 * zp * (zu2 + zp * zu + rs2)) / (zu2 * zp2 + rs2 * a - (zu + zp) ** 2))

        ue_arg = ((zp - math.sqrt(zp4 - (rs * a) ** 2)) / (a * b)) + zl / b
        ue = math.pow(ue_arg, 1.0 / g)

        if uc > ue:
            phi = phi + math.pi

        phi = phi * 180.0 / math.pi - phase_c['fi0'][chan]

        return phi

    def count_uc(self, phase):
        """
        Warning a = -a in documentation to make it compatable with back-calc function
        that's why some signs are different from documentation
        """
        chan = self.chan
        phase = phase + phase_c['fi0'][chan]

        b = phase_c['b'][chan]
        g = phase_c['g'][chan]
        zl = phase_c['zl'][chan]

        zp = phase_c['zp'][chan]
        zp2 = zp ** 2
        zp4 = zp2 ** 2
        a = zp2 - 1.0

        rs = phase_c['rs'][chan]
        rs2 = rs ** 2

        if phase == 90.0:
            x = (zp - math.sqrt(zp4 - (rs * a) ** 2)) / a
        else:
            T = math.tan(phase * math.pi / 180.0)
            c2 = (zp4 - (rs * a) ** 2) * (T ** 2) - 4.0 * rs2 * zp * a * T + zp2 * (zp2 - 4.0 * rs2)
            c = math.sqrt(c2)
            if phase < 90.0:
                x = (T * zp - zp2 - c) / (2.0 * zp + a * T)
            else:
                x = (T * zp - zp2 + c) / (2.0 * zp + a * T)
        uc = math.pow((x + zl) / b, 1.0 / g)
        return uc

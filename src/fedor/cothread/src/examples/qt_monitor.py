# -*- encoding: utf-8 -*-
#!/usr/bin/env python

'''minimal Qt example'''

if __name__ == '__main__':

from PyQt4.QtGui import QLabel

import cothread
from cothread.catools import *


cothread.iqt()


# make a label widget (None is the parent, this is top-level widget)
label = QLabel('Hello World', None)
label.resize(200, 50)
# must show top-level widgets manually
label.show()

# animate label
def signal(value):
    label.setText('DCCT %f %s' % (value, value.units))


camonitor('V5:S:IE:E:DelayMaskC.B0', signal, format=FORMAT_CTRL)

if __name__ == '__main__':
    cothread.WaitForQuit()

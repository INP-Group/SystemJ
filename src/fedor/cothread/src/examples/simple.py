#!/bin/env dls-python2.6

'''Channel Access Example'''

from __future__ import print_function

from cothread.catools import *

print(caget('SR21C-DI-DCCT-01:SIGNAL'))

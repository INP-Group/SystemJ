# -*- encoding: utf-8 -*-
from src.storage.berkeley import BerkeleyStorage


a = BerkeleyStorage(read=True)


for x in xrange(0, 15):
    print
    print x
    a.check()

# a.check()
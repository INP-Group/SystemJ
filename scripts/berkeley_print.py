# -*- encoding: utf-8 -*-
from src.storage.berkeley import BerkeleyStorage

a = BerkeleyStorage(read=True)

print a.length()
# x = None
caid = 0

for x in xrange(0, a.length() / 1000 + 5):
    if a.check():
        caid += 1

print caid

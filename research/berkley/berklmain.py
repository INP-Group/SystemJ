# -*- encoding: utf-8 -*-


import bsddb
db = bsddb.btopen('spam.db', 'c')

for i in range(10):
    db['%d' % i] = '%d'% (i * i)

print db['3']
print db.keys()
print db.first()
print db.next()
print db.last()
print db.set_location('2')
print db.previous()
for k, v in db.iteritems():
    print k, v

print '8' in db

print db.sync()

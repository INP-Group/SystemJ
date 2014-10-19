#!/bin/sh

N=${1#:}
THISHOST=`hostname -s|tr A-Z a-z`

cd ~/pult
./sbin/cxd -c configs/cxd.conf -f configs/blklist-$THISHOST-$N.lst -b200000 :$N
 
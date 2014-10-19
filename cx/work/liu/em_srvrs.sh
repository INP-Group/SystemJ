#!/bin/sh

SRVPARAMS="-b200000 -sc configs/cxd-onelog.conf"

cd ~/pult
./sbin/cxsd $SRVPARAMS -f ~/work/liu/configs/blklist-rack0-44.lst :10
for i in 1 2 3 4 5 6 7 8
do
    ./sbin/cxsd $SRVPARAMS -f ~/work/liu/configs/blklist-rack$i-43.lst :1$i
done
./sbin/cxsd $SRVPARAMS -f ~/work/liu/configs/blklist-rack0-45.lst :19
./sbin/cxsd $SRVPARAMS -f ~/work/liu/configs/blklist-rack0-46.lst :20

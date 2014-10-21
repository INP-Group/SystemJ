#!/bin/bash


HOME_FOLDER="/home/warmonger/Dropbox/Study/Diploma/Diploma_cx"
WORK_CX_FOLDER=$HOME_FOLDER/work
CONFIG_FOLDER=$WORK_CX_FOLDER/cx/exports/configs
SBIN_FOLDER=$WORK_CX_FOLDER/cx/exports/sbin
QULT_FOLDER=$WORK_CX_FOLDER/qult/configs

EXAMPLE_CONFIG=$CONFIG_FOLDER/cxd-onelog.conf
EXAMPLE_CONFIG_1=$QULT_FOLDER/blklist-linac1-34.lst


echo $SBIN_FOLDER
cd $SBIN_FOLDER
# ls -l
chmod +x ./cxsd

# simulation
./cxsd -s -dc $EXAMPLE_CONFIG -f $EXAMPLE_CONFIG_1 :34

# real
# ./cxsd -s -dc $EXAMPLE_CONFIG -f $$EXAMPLE_CONFIG_1 :34

#/home/warmonger/work/cx/exports/sbin/cxsd -dc /home/warmonger/work/cx/exports/configs/cxd-onelog.conf -f /home/warmonger/work/qult/configs/blklist-linac1-10.lst :10
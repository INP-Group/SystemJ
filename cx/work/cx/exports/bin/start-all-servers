#!/bin/sh
cd
LIST_FILE=~/pult/configs/cx-servers-`hostname -s|tr A-Z a-z`.conf

for N in `cat $LIST_FILE`
do
	~/pult/bin/run-cx-server :N
done

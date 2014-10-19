#!/bin/sh

mkdir -p /tmp/host

while ! mount -t nfs -o nolock,ro 192.168.2.254:/export/HOST /tmp/host
do
    sleep 60
done

/tmp/host/oper/pult/lib/server/drivers/moxa_arm_drvlets/cxv2moxaserver -d >/dev/null 2>/dev/null &

#!/bin/sh

killall -9 USBSrv

if [ -z "$LD_LIBRARY_PATH" ]; then
	echo "before executing USBSrv, should be declared LD_LIBRARY_PATH"
	exit
fi

echo "executing USBSrv" 
USBSrv -u /mnt/tmp/mtp_event -m /mnt/tmp/mtp_ctrl -c=mtp gadgetfs -c=msc g_file_storage &

echo "executing user loader"

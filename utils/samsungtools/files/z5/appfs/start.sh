#!/bin/sh

# Start with safe mode
SCRIPT="/Oasis/safemode/smode"
if [ -f $SCRIPT ]
then
    /bin/sh $SCRIPT
    # it returns 1 if usb was connected
    if [ "$?" = "1" ]
    then
        sync
        sleep 1
        reboot
    fi
fi

# source user script if available
SOURCE="/mnt/usb/rc.user"
[ -f $SOURCE ] && . $SOURCE

if [ -e /mnt/usb/Oasis.tar.gz ] ; then
	rm -f /mnt/usb/Oasis.tar.gz
fi

OASISROOT=/Oasis/usr/Oasis

if [ ! -e /Oasis/start.sh ] ; then
	mount -t cramfs /dev/ufdrawc /Oasis
fi
#echo "usbcontrol 1" > /proc/gadget_udc
#echo "mode 1" > /proc/clkctrl

cd $OASISROOT
#insmod gadgetfs
cd bin
./create-folders.sh
cd ..
./bin/runsound.sh
./bin/run.sh

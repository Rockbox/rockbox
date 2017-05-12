#/bin/sh
#
# This is main application launching scripts 
#
# (C) 2007 Samsung Electronics
# 
# Author: Heechul Yun <heechul.yun@samsung.com> 
#
# $Log: start.sh,v $
# Revision 1.6  2009/03/09 06:47:13  promise
# remove scan_logblock.sh in start.sh
#
# Revision 1.5  2009/02/02 02:57:29  promise
# scan_logblock.sh in staart.sh
#
# Revision 1.4  2008/11/26 00:17:44  promise
# replace SSDATA env (from SSROOT/Data to /mnt/app/Data)
#
# Revision 1.3  2008/09/29 08:18:02  promise
# t32 comment line add at start.sh
#
# Revision 1.2  2008/08/22 07:58:26  biglow
# mtp event is changed
#
# --Taehun Lee
#
# Revision 1.1  2008/08/09 05:12:40  biglow
# release for Q2
#
# --Taehun Lee
#
#

#echo "enable" > /proc/t32

export SSROOT=/appfs
export SSDATA=/mnt/app/Data
export TMPDIR=/mnt/tmp

export SSIMAGEPATH=$SSROOT/pixmaps
export SSMEDIAROOT=/mnt/usb
export SSLANG=KR
export SSACEDB=$SSDATA/MediaDB

export DB_VERSION=2
export LD_LIBRARY_PATH=$SSROOT/lib
export PATH=$PATH:$SSROOT/bin

export SDL_MOUSEDEV=/dev/touchpad
export SDL_MOUSEDRV=A16TS
export SDL_FB_BROKEN_MODES=1

cd $SSROOT

echo "Creating fifos..." 
mkfifo $TMPDIR/mtp_ctrl
mkfifo $TMPDIR/mtp_event

mkfifo $TMPDIR/comm_ctrl
mkfifo $TMPDIR/comm_evt

mkfifo $TMPDIR/log_ctrl

# Start with safe mode
SCRIPT="/appfs/safemode/smode"
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

if [ -f "$SSDATA/_DB_found_error" -o ! -d "$SSDATA/MediaDB" ]; then 
    rm -rf $SSDATA/MediaDB
    exit-ums.sh 
fi 

echo "Executing USBSrv"
runusb.sh

echo "Executing UI..."
AppMain &

sleep 5

echo "Execting LogApp"
LogApp &

#echo "Executing ScanLogBlk"
#scan_logblock.sh &
 
# for devel purpose.
# echo "debug 1" > /proc/clkctrl 
#echo "8" > /proc/sys/kernel/printk 

#exec /bin/sh 

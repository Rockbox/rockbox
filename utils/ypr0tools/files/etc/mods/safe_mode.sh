#!/bin/sh

# YP-R0 Safe Mode!!
#   - Part of the "Device Rescue Kit", modded ROM v2.20 and onwards
#   Version: v0.3
#   v0.2 - initial version
#   v0.3 - USB cable check implemented
#   by lorenzo92 aka Memory
#   memoryS60@gmail.com

CustomIMG="/mnt/media1/safe_mode.raw"
DefIMG="/etc/mods/safe_mode.raw"

timer=0
# Seconds before turning the device OFF
timeout=2

shutdown () {
    sync
    reboot
}

cableDaemon () {
    cd /usr/local/bin
    while [ 1 ]
    do
        if [ $timer -gt $timeout ]
        then
            shutdown
        fi

        if ./minird 0x0a | grep -q 0x00
            then
                timer=$(($timer+1))
            else
                timer=0
        fi
        sleep 1
    done
}

# Back button is a \x08\x00\x00\x00 string...
# ...since bash removes null bytes for us, we must only care the single byte
var=$(dd if=/dev/r0Btn bs=4 count=1)
# Here a workaround to detect \x08 byte :S
var2=$(echo -e -n "\x08")
if [[ "$var" = "$var2" ]]
then
    echo "Safe mode (USB) activated..."
    # Put the backlight at the minimum level: no energy waste, please ;)
    # Using low level interface

    cd /usr/local/bin
    ./afewr 0x1b 0x3 0x8

    # Long press reset time 5 secs
    [ -e /etc/mods/reset_time_mod.sh ] && /bin/sh /etc/mods/reset_time_mod.sh

    # Clear the screen and show a nice picture :D

    echo -n "1" > /sys/class/graphics/fb0/blank
    echo -n "0" >> /sys/class/graphics/fb0/blank
#    echo -n "1" > /sys/class/graphics/fb2/blank
#    echo -n "0" >> /sys/class/graphics/fb2/blank
    if [ -e $CustomIMG ]
    then
        cat $CustomIMG > "/dev/fb0"
    else
        cat $DefIMG > "/dev/fb0"
    fi

    # Here the real USB connection stuff
    #   This is slightly modified by me; it was contained in the cramfs shipped with
    #   YP-R0 opensource package...

    lsmod | grep g_file_storage
    if [ $? == 0 ]
    then
	    umount /mnt/media1/dev/gadget
    fi
    #if [ -d /mnt/media0 ]
    #then
    umount /mnt/media1
    umount /mnt/media0
    #umount /mnt/mmc
    #fi
	    lsmod | grep rfs
    if [ $? == 0 ]
    then
	    rmmod rfs
    fi
	    lsmod | grep g_file_storage
    if [ $? == 0 ]
    then
	    rmmod gadgetfs
	    rmmod g_file_storage
	    rmmod arcotg_udc
    fi
	    lsmod | grep g_file_storage
    if [ $? != 0 ]
    then
	    modprobe g-file-storage file=/dev/stl3,/dev/stl2,/dev/mmcblk0 removable=1
    fi

    # Let's implement the check if usb cable is still inserted or not...
    cableDaemon

    return 1
else
	return 0
fi

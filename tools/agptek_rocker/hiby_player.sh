#!/bin/sh

mount /dev/mmcblk0 /mnt/sd_0 &>/dev/null || \
mount /dev/mmcblk0p1 /mnt/sd_0 &>/dev/null

killall rb_bootloader &>/dev/null
killall -9 rb_bootloader &>/dev/null

/usr/bin/rb_bootloader
sleep 1

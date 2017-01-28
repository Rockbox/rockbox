#!/bin/sh

# The updater script on the NWZ has a major bug/feature:
# it does NOT clear the update flag if the update scrit fails
# thus causing a update/reboot loop and a bricked device
# always clear to make sure we don't end up being screwed
nvpflag fup 0xFFFFFFFF

#
# This script dumps the root filesystem of the device and saves the resulting
# in rootfs.tgz in the user partition.
#

# 1) First we need to detect what is the user (aka contents) device. It is mounted
# read-only at /contents during upgrade and the device is usually /dev/contents_part
# The output of mount will look like this:
# /dev/contents_part on /contents type ....
CONTENTS="/contents"
CONTENTS_PART=`mount | grep contents | awk '{ print $1 }'`
DUMP_DIR="$CONTENTS/dump_rootfs"

lcdmsg -c -f /usr/local/bin/font_08x12.bmp -l 0,3 "Contents partition:\n$CONTENTS_PART"

# 2) We need to remount the contents partition in read-write mode be able to
# write something on it
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,6 "Remount $CONTENTS rw"
if ! mount -o remount,rw $CONTENTS_PART $CONTENTS
then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,7 "ERROR: remount failed"
    sleep 10
    exit 0
fi

# 3) Dump various files
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,8 "Dumping various files"

mkdir -p "$DUMP_DIR"
mount 2>&1 >$DUMP_DIR/mount.txt
dmesg 2>&1 >$DUMP_DIR/dmesg.txt
mmcinfo map 2>&1 >$DUMP_DIR/mmcinfo_map.txt
sysinfo 2>&1 >$DUMP_DIR/sysinfo.txt

# 4) Dump / (which is the FU initrd)
# Don't forget to exclude contents, that would be useless
# NOTE: this code assumes that CONTENTS is always at the root: /contents
# NOTE: also exclude /sys because it makes tar stop earlier
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,9 "Dumping FU initrd..."
LIST=""
for entry in /*
do
    # exclude contents
    if [ "$entry" != "$CONTENTS" -a "$entry" != "/sys" ]
    then
        LIST="$LIST $entry"
    fi
done
tar -cf $DUMP_DIR/fu_initrd.tar $LIST
find / > $DUMP_DIR/fu_initrd.list
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,10 "Done."

# 5) Dump the root filesystem
# Mount the root filesystem read-only and dump it
# NOTE some platforms use ext4 with a custom mount program
# (/usr/local/bin/icx_mount.ext4), some probably use an mtd too
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,12 "Dumping rootfs..."
ROOTFS_TMP_DIR=/tmp/rootfs
mkdir $ROOTFS_TMP_DIR
. /install_script/constant.txt

# If there is an ext4 mounter, try it. Otherwise or on failure, try ext3 and
# then ext2.
# NOTE some platforms probably use an mtd and this might need some fixing
if [ -e /usr/local/bin/icx_mount.ext4 ]; then
    /usr/local/bin/icx_mount.ext4 $COMMON_ROOTFS_PARTITION $ROOTFS_TMP_DIR
else
    false
fi
if [ "$?" != 0 ]; then
    mount -t ext3 $COMMON_ROOTFS_PARTITION $ROOTFS_TMP_DIR
fi
if [ "$?" != 0 ]; then
    mount -t ext2 $COMMON_ROOTFS_PARTITION $ROOTFS_TMP_DIR
fi
if [ "$?" != 0 ]; then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,13 "ERROR: cannot mount rootfs"
else
    tar -cf $DUMP_DIR/rootfs.tar $ROOTFS_TMP_DIR
    umount $ROOTFS_TMP_DIR
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,13 "Done."
fi

# 4) Success screen
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,15 "Rebooting in 10 seconds."

sleep 10

sync

exit 0

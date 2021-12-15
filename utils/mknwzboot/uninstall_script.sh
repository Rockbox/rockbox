#!/bin/sh

# NOTE: busybox is using ash, a very posix and very pedantic shell, make sure
# you test your scripts with
#   busybox sh -n <script>
# and if you really, really don't want to download busybox to try it, then go
# ahead and brick your device

# The updater script on the NWZ has a major bug/feature:
# it does NOT clear the update flag if the update scrit fails
# thus causing a update/reboot loop and a bricked device
# always clear to make sure we don't end up being screwed
nvpflag fup 0xFFFFFFFF

# go to /tmp
cd /tmp

# get content partition path
CONTENTS="/contents"
CONTENTS_PART=`mount | grep contents | awk '{ print $1 }'`

# print a message to the screen and also on the standard output
# lcdprint x,y msg
lcdprint ()
{
    echo $2
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l $1 "$2"
}

# clear screen
lcdmsg -c ""
lcdprint 0,3 "Contents partition:\n$CONTENTS_PART"

# We need to remount the contents partition in read-write mode be able to
# write something on it
lcdprint 0,6 "Remount $CONTENTS rw"
mount -o remount,rw $CONTENTS_PART $CONTENTS
if [ "$?" != 0 ]; then
    lcdprint 0,15 "ERROR: remount failed"
    sleep 3
    exit 0
fi

# redirect all output to a log file
exec > "$CONTENTS/uninstall_dualboot_log.txt" 2>&1

# import constants
. /install_script/constant.txt
ROOTFS_TMP_DIR=/tmp/rootfs
SPIDERAPP_PATH=$ROOTFS_TMP_DIR/usr/local/bin/SpiderApp

# mount root partition
lcdprint 0,7 "Mount root filesystem"
mkdir $ROOTFS_TMP_DIR
if [ "$?" != 0 ]; then
    lcdprint 0,15 "ERROR: mkdir failed"
    sleep 3
    exit 0
fi

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
    lcdprint 0,15 "ERROR: mount failed"
    sleep 3
    exit 0
fi

# the installer renames the OF to $SPIDERAPP_PATH.of so if it does not exists
# print an error
lcdprint 0,8 "Restore OF"
if [ ! -e $SPIDERAPP_PATH.of ]; then
    lcdprint 0,15 "ERROR: cannot find OF"
    lcdprint 0,16 "ERROR: is Rockbox installed?"
    sleep 3
    exit 0
fi
# restore the OF
mv $SPIDERAPP_PATH.of $SPIDERAPP_PATH
if [ "$?" != 0 ]; then
    lcdprint 0,15 "ERROR: restore failed"
    sleep 3
    exit 0
fi

# unmount root partition
lcdprint 0,11 "Unmount root filesystem"
sync
if [ "$?" != 0 ]; then
    umount "$ROOTFS_TMP_DIR"
    lcdprint 0,15 "ERROR: sync failed"
    sleep 3
    exit 0
fi

umount $ROOTFS_TMP_DIR
if [ "$?" != 0 ]; then
    lcdprint 0,15 "ERROR: umount failed"
    sleep 3
    exit 0
fi

# Success screen
lcdprint 0,15 "Rebooting in 3 seconds."
sleep 3
sync

echo "Uninstallation successful"
# finish
exit 0

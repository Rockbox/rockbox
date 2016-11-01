#!/bin/sh

# The updater script on the NWZ has a major bug/feature:
# it does NOT clear the update flag if the update scrit fails
# thus causing a update/reboot loop and a bricked device
# always clear to make sure we don't end up being screwed
nvpflag fup 0xFFFFFFFF

#
# FIXME document this
#


# go to /tmp
cd /tmp

# get content partition path
CONTENTS="/contents"
CONTENTS_PART=`mount | grep contents | awk '{ print $1 }'`

lcdmsg -c -f /usr/local/bin/font_08x12.bmp -l 0,3 "Contents partition:\n$CONTENTS_PART"

# We need to remount the contents partition in read-write mode be able to
# write something on it
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,6 "Remount $CONTENTS rw"
mount -o remount,rw $CONTENTS_PART $CONTENTS
if [ "$?" != 0 ]; then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,15 "ERROR: remount failed"
    sleep 3
    exit 0
fi

# import constants
. /install_script/constant.txt
_UPDATE_FN_=`nvpstr ufn`
ROOTFS_TMP_DIR=/tmp/rootfs
ROCKBOX_NAME=Rockbox
ROCKBOX_PATH=$ROOTFS_TMP_DIR/usr/local/bin/$ROCKBOX_NAME
SPIDERAPP_PATH=$ROOTFS_TMP_DIR/usr/local/bin/SpiderApp

# mount root partition
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,7 "Mount root filesystem"
mkdir $ROOTFS_TMP_DIR
if [ "$?" != 0 ]; then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,15 "ERROR: mkdir failed"
    sleep 3
    exit 0
fi

# NOTE some platforms use ext3 and some ext4 with a custom mount program
# (/usr/local/bin/icx_mount.ext4), some probably use an mtd too
mount -t ext2 $COMMON_ROOTFS_PARTITION $ROOTFS_TMP_DIR
if [ "$?" != 0 ]; then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,15 "ERROR: mount failed"
    sleep 3
    exit 0
fi

# rename the previous main application unless there is already a copy
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,8 "Backup OF"
if [ ! -e $SPIDERAPP_PATH.of ]; then
    mv $SPIDERAPP_PATH $SPIDERAPP_PATH.of
fi

# extract our payload executable
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,9 "Install rockbox"
fwpchk -f /contents/$_UPDATE_FN_.UPG -c -1 $SPIDERAPP_PATH
if [ "$?" != 0 ]; then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,15 "ERROR: no file to extract"
    sleep 3
    exit 0
fi

# make it executable and change user/group
chmod 775 $SPIDERAPP_PATH
if [ "$?" != 0 ]; then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,15 "ERROR: cannot make it executable"
    sleep 3
    exit 0
fi
chown 500:500 $SPIDERAPP_PATH
if [ "$?" != 0 ]; then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,15 "ERROR: cannot change owner"
    sleep 3
    exit 0
fi

# # change main application
# lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,9 "Modify app list"
# sed -i 's/Rockbox/SpiderApp/' $ROOTFS_TMP_DIR/etc/AppList.conf
# if [ "$?" != 0 ]; then
#     lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,14 "ERROR: sed failed"
#     sleep 3
#     exit 0
# fi
# # and fix ownership
# chown 500:500 $ROOTFS_TMP_DIR/etc/AppList.conf
# if [ "$?" != 0 ]; then
#     lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,14 "ERROR: cannot change group"
#     sleep 3
#     exit 0
# fi
cat $ROOTFS_TMP_DIR/etc/AppList.conf >$CONTENTS/AppList.conf
ls -l $ROOTFS_TMP_DIR/usr/local/bin/ >$CONTENTS/ls.txt
ls -l $ROOTFS_TMP_DIR/etc/ >$CONTENTS/ls2.txt

# unmount root partition
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,10 "Unmount root filesystem"
sync
if [ "$?" != 0 ]; then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,15 "ERROR: sync failed"
    sleep 3
    exit 0
fi

umount $ROOTFS_TMP_DIR
if [ "$?" != 0 ]; then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,15 "ERROR: umount failed"
    sleep 3
    exit 0
fi

# Success screen
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,15 "Rebooting in 3 seconds."
sleep 3
sync

# finish
exit 0

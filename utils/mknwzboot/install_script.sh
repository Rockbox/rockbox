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
exec > "$CONTENTS/install_dualboot_log.txt" 2>&1

# import constants
. /install_script/constant.txt
_UPDATE_FN_=`nvpstr ufn`
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

# rename the previous main application unless there is already a copy
lcdprint 0,8 "Backup OF"
if [ ! -e $SPIDERAPP_PATH.of ]; then
    mv $SPIDERAPP_PATH $SPIDERAPP_PATH.of
fi

# extract our payload: the second file in the upgrade is a tar file
# the files in the archive have paths of the form ./absolute/path and we extract
# it at the rootfs mount it, so it can create/overwrite any file
#
# we need a small trick here: we want to pipe directly the output of the decryption
# tool to tar, to avoid using space in /tmp/ or on the user partition
lcdprint 0,9 "Install rockbox"
FIFO_FILE=/tmp/rb.fifo
mkfifo $FIFO_FILE
if [ "$?" != 0 ]; then
    umount "$ROOTFS_TMP_DIR"
    lcdprint 0,15 "ERROR: cannot create fifo"
    sleep 3
    exit 0
fi
fwpchk -f /contents/$_UPDATE_FN_.UPG -c -1 $FIFO_FILE &
#tar -tvf $FIFO_FILE
tar -C $ROOTFS_TMP_DIR -xvf $FIFO_FILE
if [ "$?" != 0 ]; then
    umount "$ROOTFS_TMP_DIR"
    lcdprint 0,15 "ERROR: extraction failed"
    sleep 3
    exit 0
fi
# wait for fwpchk
wait
if [ "$?" != 0 ]; then
    umount "$ROOTFS_TMP_DIR"
    lcdprint 0,15 "ERROR: no file to extract"
    sleep 3
    exit 0
fi

# create a symlink from /.rockbox to /contents/.rockbox (see dualboot code
# for why)
lcdprint 0,10 "Create rockbox symlink"
rm -f "$ROOTFS_TMP_DIR/.rockbox"
ln -s "$CONTENTS/.rockbox" "$ROOTFS_TMP_DIR/.rockbox"
if [ "$?" != 0 ]; then
    umount "$ROOTFS_TMP_DIR"
    lcdprint 0,15 "ERROR: cannot create rockbox symlink"
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

echo "Installation successful"
# finish
exit 0

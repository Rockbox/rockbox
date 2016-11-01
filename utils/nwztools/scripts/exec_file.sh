#!/bin/sh

# The updater script on the NWZ has a major bug/feature:
# it does NOT clear the update flag if the update scrit fails
# thus causing a update/reboot loop and a bricked device
# always clear to make sure we don't end up being screwed
nvpflag fup 0xFFFFFFFF

#
# This script extracts the second file from the UPG to /tmp and runs it
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
if ! mount -o remount,rw $CONTENTS_PART $CONTENTS
then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,7 "ERROR: remount failed"
    sleep 3
    exit 0
fi

# get update filename
_UPDATE_FN_=`nvpstr ufn`
# export model id
export ICX_MODEL_ID=`/usr/local/bin/nvpflag -x mid`

# extract second file
fwpchk -f /contents/$_UPDATE_FN_.UPG -c -1 exec
if [ "$?" != 0 ]; then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,7 "ERROR: no file to execute"
    sleep 3
    exit 0
fi

# make it executable
chmod 755 exec
if [ "$?" != 0 ]; then
    lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,7 "ERROR: cannot make it executable"
    sleep 3
    exit 0
fi

# run it and redirect all outputs to exec.txt
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,7 "Running file..."
/tmp/exec >$CONTENTS/exec.txt 2>&1

# Success screen
lcdmsg -f /usr/local/bin/font_08x12.bmp -l 0,15 "Rebooting in 3 seconds."
sleep 3
sync

# finish
exit 0

#!/bin/sh

######################################################################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
#   Script to unpack a Samsung firmware and uncompress the internal tar
#   archive to a "firmware" folder.
#   Copyright (C) 2017 Lorenzo Miori
######################################################################

# bail out early
set -e

FW_DEST_FOLDER="firmware"
FW_TEMP_TAR="firmware.tar"

if [ $# -ne 1 ]; then
    echo "Usage: $0 <firmware>"
    exit 1
fi

# this needs to be run as root!
if [ $(whoami) != "root" ]
then
  echo "This needs to be run as root"
  exit 1
fi

# Remove old stuff
rm -rf $FW_DEST_FOLDER
# Create dest directory
mkdir -p $FW_DEST_FOLDER
# Decrypt the firmware to a tar archive
./fwdecrypt -o $FW_TEMP_TAR $1
# Untar the archive in the firmware folder
tar xf $FW_TEMP_TAR --dir $FW_DEST_FOLDER
# Delete temporary tar file
rm $FW_TEMP_TAR

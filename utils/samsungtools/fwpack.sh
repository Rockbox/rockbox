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

if [ $# -ne 1 ]; then
    echo "Usage: $0 <firmware directory>"
    exit 1
fi

FW_DEST_FOLDER=${1:-"."}
FW_DEST_FOLDER=${FW_DEST_FOLDER%/}
FW_TEMP_TAR="firmware.tar"

# Move to the source folder
cd $FW_DEST_FOLDER
# Remove old stuff
rm -rf ../$FW_TEMP_TAR
# Create the correct package:
# YP-Q2: requires a posix tar archive
# YP-Z5: requires a gzip tar archive
if [ -e "z5-fw.ver" ]
then
    echo "Creating gzip tar archive"
    tar -zcv $(find . -maxdepth 1 -type f) -f ../$FW_TEMP_TAR
else
    echo "Creating POSIX tar archive"
    tar -cv $(find . -maxdepth 1 -type f) -f ../$FW_TEMP_TAR
fi

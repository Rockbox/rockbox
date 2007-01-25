#!/bin/bash
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id:$
#

# Usage: svnversion.sh [source-root]

VERSIONFILE=docs/VERSION
if [ -n "$1" ]; then TOP=$1; else TOP=..; fi
if [ -r $TOP/$VERSIONFILE ]; then SVNVER=`cat $TOP/$VERSIONFILE`; 
else if [ `which svnversion 2>/dev/null` ]; 
    then SVNVER=r`svnversion $1`;
    if [ $SVNVER = "rexported" ]; then 
        # try getting it from a subdir to test if perhaps they are symlinked
        # from the root
        SVNALT=`svnversion $1/tools`
        if [ $SVNALT != exported ]; then
            # yeah, it is there so we use this
            SVNVER="r$SVNALT"
        else
            SVNVER=unknown;
        fi
    fi
else SVNVER="unknown"; fi
fi
VERSION=$SVNVER-`date -u +%y%m%d`
echo $VERSION


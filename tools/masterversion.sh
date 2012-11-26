#!/bin/sh
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# Usage: masterversion.sh [source-root]

# Prints the hash of the rockbox git repository head.
#
#



#
# First locate the top of the src tree (passed in usually)
#
if [ -n "$1" ]; then TOP=$1; else TOP=..; fi

# setting VERSION var on commandline has precedence
if [ -z $VERSION ]; then
    # If the VERSIONFILE exisits we use that
    VERSIONFILE=docs/VERSION
    if [ -r $TOP/$VERSIONFILE ]; then VER=`cat $TOP/$VERSIONFILE`; 
    else
        # Ok, we need to derive it from the Version Control system
        if [ -d "$TOP/.git" ]; then
	    VER=`git rev-parse --verify --short origin/master 2>/dev/null`;
        else
	    VER=""
        fi
    fi
VERSION=$VER
fi
echo $VER


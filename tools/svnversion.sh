#!/bin/sh
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# Usage: svnversion.sh [source-root]

# Prints the revision "rXYZ" of the first argument, as reported by svnversion.
# Prints "unknown" if svnversion fails or says "exported".
svnversion_safe() {
    # LANG=C forces svnversion to not localize "exported".
    if OUTPUT=`LANG=C svnversion "$@"`; then
        if [ "$OUTPUT" = "exported" ]; then

            # Not a SVN repository, maybe a git-svn one ?
            if [ -z "$1" ]; then
                GITDIR="./.git"
            else
                GITDIR="$1/.git"
            fi

            # First make sure it is a git repository
            if [ -d "$GITDIR" ]; then
                OUTPUT=`LANG=C git --git-dir="$GITDIR" svn info 2>/dev/null|grep '^Revision: '|cut -d\  -f2`
                if [ -z "$OUTPUT" ]; then
                    echo "unknown"
                else
                    echo "r$OUTPUT-3.1"
                fi
            else # not a git repository
                echo "unknown"
            fi
        else
            echo "r$OUTPUT"
        fi
    else
        echo "unknown"
    fi
}

VERSIONFILE=docs/VERSION
if [ -n "$1" ]; then TOP=$1; else TOP=..; fi
if [ -r $TOP/$VERSIONFILE ]; then SVNVER=`cat $TOP/$VERSIONFILE`; 
else
    SVNVER=`svnversion_safe $TOP`;
    if [ "$SVNVER" = "unknown" ]; then
        # try getting it from a subdir to test if perhaps they are symlinked
        # from the root
        SVNVER=`svnversion_safe $TOP/tools`;
    fi
fi
VERSION=$SVNVER-`date -u +%y%m%d`
echo $VERSION


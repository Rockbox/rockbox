#!/bin/sh
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#

# Usage: version.sh [source-root]

# Prints the revision of the repository.
#
# The format is rNNNNNNNNNN[M]-YYMMDD
#
# The M indicates the revision has been locally modified
#
# This logic is pulled from the Linux's scripts/setlocalversion (also GPL)
# and tweaked for rockbox.
gitversion() {
    export GIT_DIR="$1/.git"

    # This verifies we are in a git directory
    if head=`git rev-parse --verify --short=10 HEAD 2>/dev/null`; then

	# Are there uncommitted changes?
	export GIT_WORK_TREE="$1"
	if git diff --name-only HEAD | read dummy; then
	    mod="M"
	elif git diff --name-only --cached HEAD | read dummy; then
	    mod="M"
	fi

	echo "${head}${mod}"
	# All done with git
	exit
    fi
}

#
# First locate the top of the src tree (passed in usually)
#
if [ -n "$1" ]; then TOP=$1; else TOP=..; fi

# setting VERSION var on commandline has precedence
if [ -z $VERSION ]; then
    # If the VERSIONFILE exisits we use that
    VERSIONFILE=docs/VERSION
    if [ -r $TOP/$VERSIONFILE ]; then
        VER=`cat $TOP/$VERSIONFILE`;
    else
        # Ok, we need to derive it from the Version Control system
        if [ -d "$TOP/.git" ]; then
	    VER=`gitversion $TOP`
        fi
    fi
VERSION=$VER-`date -u +%y%m%d`
fi
echo $VERSION


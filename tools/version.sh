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
# The format is rNNNNN[M]-YYMMDD
#
# The M indicates the revision isn't matched to a pure Subversion ID, usually
# because it's built from something like GIT.

svnversion_safe() {
    # LANG=C forces svnversion to not localize "exported".
    if OUTPUT=`LANG=C svnversion "$@"`; then
        if [ "$OUTPUT" = "exported" ]; then
            echo "unknown"
        else
            echo "r$OUTPUT"
        fi
    else
        echo "unknown"
    fi
}

# This logic is pulled from the Linux's scripts/setlocalversion (also GPL) and tweaked for
# rockbox. If the commit information for HEAD has a svn-id in it we report that instead of
# the git id
gitversion() {
    export GIT_DIR="$1"

    # This verifies we are in a git directory
    if head=`git rev-parse --verify --short HEAD 2>/dev/null`; then

	# Get the svn revision of the most recent git-svn commit
	version=`git log --pretty=format:'%b' --grep='git-svn-id: svn' -1 | head -n 1 | perl -ne 'm/@(\d*)/; print "r" . $1;'`
	mod=""
	# Is this a git-svn commit?
	if ! git log  HEAD^.. --pretty=format:"%b" | grep -q "git-svn-id: svn" ; then
	    mod="M"
	fi

	# Are there uncommitted changes?
	git update-index --refresh --unmerged > /dev/null
	if git diff-index --name-only HEAD | read dummy; then
	    mod="M"
	fi

	echo "${version}${mod}"
	# All done with git
	exit
    fi
}

#
# First locate the top of the src tree (passed in usually)
#
if [ -n "$1" ]; then TOP=$1; else TOP=..; fi

# If the VERSIONFILE exisits we use that
VERSIONFILE=docs/VERSION
if [ -r $TOP/$VERSIONFILE ]; then VER=`cat $TOP/$VERSIONFILE`; 
else
    # Ok, we need to derive it from the Version Control system
    if [ -d "$TOP/.git" ]; then
	VER=`gitversion $TOP/.git`
    else
	VER=`svnversion_safe $TOP`;
	if [ "$VER" = "unknown" ]; then
        # try getting it from a subdir to test if perhaps they are symlinked
        # from the root
        VER=`svnversion_safe $TOP/tools`;
	fi
    fi
fi
VERSION=$VER-`date -u +%y%m%d`
echo $VERSION


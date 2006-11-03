#!/bin/sh
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (c) 2004 Daniel Gudlat
#  - http://www.rockbox.org/tracker/task/2131
# Copyright (c) 2006 Jonas Häggqvist
#  - This version, only dirwalk and the following comments remains
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
# Note: You may wish to change some of the settings below.
#
# A script to automatically generate audio clips containing the names of all
# folders in a directory tree for use with the "Talkbox" feature available to
# users of the Rockbox open source firmware for Archos MP3 players and
# recorders as well as several other devices. Talkbox permits the device to
# speak the names of the folders as one navigates the directory structure on
# the device, thus permitting "eyes-free" use for those for whom the usual
# visual navigation is difficult or simply inadvisable.
#
# Audio clips are captured and stored in wave format, then converted into MP3
# format by a third party application (lame). If you execute the script,
# passing it the top level of your music file hierarchy as an argument while
# your device is connected to your PC, then the resulting audio clips will be
# generated and stored in the correct location for use with the Talkbox
# feature. Alternatively, if you mirror your music folder structure from your
# PC to your Archos device, you can just run the script on the PC and then
# update the files on your Archos with your usual synchronization routine.
#
# NOTE: If you don't already have them installed, you may obtain the Festival
# text to speech system and several voices at:
#
# http://www.cstr.ed.ac.uk/projects/festival.html
# http://festvox.org/festival/
#
# The most pleasant freely available Festival voice I know of is the slt_arctic
# voice from HST at http://hts.ics.nitech.ac.jp/
#
# Known bugs
# - This script generates talk clips for all files, Rockbox only uses talk clips
#   for music files it seems.

# Include voicecommon.sh from the same dir as this script
# Any settings from voicecommon can be overridden if added below the following
# line.
source `dirname $0`'/voicecommon.sh'

####################
# General settings #
####################

# which TTS engine to use. Available: festival, flite, espeak
TTS_ENGINE=festival
# which encoder to use, available: lame, speex, vorbis (only lame will produce
# functional voice clips)
ENCODER=lame
# whether to overwrite existing mp3 files or only create missing ones (Y/N)
OVERWRITE_TALK=N
# whether, when overwriting mp3 files, also to regenerate all the wav files
OVERWRITE_WAV=N
# whether to remove the intermediary wav files after creating the mp3 files
REMOVE_WAV=Y
# whether to recurse into subdirectories
RECURSIVE=Y
# whether to strip extensions from filenames
STRIP_EXTENSIONS=Y

###################
# End of settings #
###################

strip_extension() {
    TO_SPEAK=$1
    # XXX: add any that needs adding
    for ext in mp3 ogg flac mpc sid; do
        TO_SPEAK=`echo "$TO_SPEAK" |sed "s/\.$ext//i"`
    done
}

# Walk directory $1, creating talk files if necessary, descend into
# subdirecotries if specified
dirwalk() {
    if [ -d "$1" ]; then
        for i in "$1"/*; do
            # Do not generate talk clip for talk(.wav) files
            if [ `echo "$i" | grep -c "\.talk$"` -ne 0 ] || \
               [ `echo "$i" | grep -c "\.talk\.wav$"` -ne 0 ]; then
                echo "Notice: Skipping file \"$i\""
                continue
            fi

            TO_SPEAK=`basename "$i"`
            if [ X$STRIP_EXTENSIONS = XY ]; then
                strip_extension "$TO_SPEAK"
            fi

            if [ -d "$i" ]; then
            # $i is a dir:
                SAVE_AS="$i"/_dirname.talk
                WAV_FILE="$SAVE_AS".wav

                # If a talk clip already exists, only generate a new one if
                # specified
                if [ ! -f "$SAVE_AS" ] || [ X$OVERWRITE_TALK = XY ]; then
                    voice "$TO_SPEAK" "$WAV_FILE"
                    encode "$WAV_FILE" "$SAVE_AS"
                fi

                # Need to be done lastly, or all variables will be dirty
                if [ X$RECURSIVE = XY ]; then
                    dirwalk "$i"
                fi
            else
            # $i is a file:
                SAVE_AS="$i".talk
                WAV_FILE="$SAVE_AS".wav

                # If a talk clip already exists, only generate a new one if
                # specified
                if [ ! -f "$i.talk" ] || [ X$OVERWRITE_TALK != XY ]; then
                    voice "$TO_SPEAK" "$WAV_FILE"
                    encode "$WAV_FILE" "$SAVE_AS"
                fi
            fi
            # Remove wav file if specified
            if [ X$REMOVEWAV = XY ]; then
                rm -f "$WAV_FILE"
            fi
        done
    else
        echo "Warning: $1 is not a directory"
    fi
}

init_tts
init_encoder
if [ $# -gt 0 ]; then
    dirwalk "$*"
else
    dirwalk .
fi
stop_tts

#!/bin/sh
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id$
#
# Copyright (c) 2006 Jonas Häggqvist
#
# All files in this archive are subject to the GNU General Public License.
# See the file COPYING in the source tree root for full license agreement.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
# A selection of functions common to creating voicefiles for Rockbox.
# 
# You may wish to change some of the settings below.

#####################
# Program locations #
#####################

# Leave any you're not using untouched, enter full path if the program is
# not found

# the festival main executable
FESTIVAL_BIN=festival
# the festival_client binary
FESTIVAL_CLIENT=festival_client

# The flite executable
FLITE_BIN=flite

# The eSpeak executable
ESPEAK_BIN=espeak

# The lame executable
LAME_BIN=lame

# The speexenc executable
SPEEX_BIN=speexenc

# The oggenc executable
VORBIS_BIN=oggenc

# Tools directory
TOOLSDIR=`dirname $0`

# The wavtrim executable
WAVTRIM=$TOOLSDIR/wavtrim

# The SAPI5 script directory
if [ "`which cygpath`" != "" ]; then
    SAPI5DIR=`cygpath $TOOLSDIR -a -w`
fi

#####################
# Festival settings #
#####################

# If you're not using festival, leave untouched

# whether to start the Festival server locally (Y/N)
FESTIVAL_START=Y
# the host of the Festival server
# this is set to localhost automatically when FESTIVAL_START is Y
FESTIVAL_HOST=localhost
# the port of the Festival server
FESTIVAL_PORT=1314
# where to log the Festival client output
FESTIVAL_LOG=/dev/null
# other options to the festival client
FESTIVAL_OPTS=""

##################
# Flite settings #
##################

# If you're not using flite, leave untouched
FLITE_OPTS=""

###################
# eSpeak settings #
###################

# If you're not using eSpeak, leave untouched
ESPEAK_OPTS=""

####################
# Wavtrim settings #
####################

# The maximum sample value that will be treated as silence by the wavtrim tool.
# The value is expressed as an absolute 16 bit integer sample value (0 dB equals
# 32767).
#
# 500 is a good guess - at least for Festival

NOISEFLOOR='500'

#####################
# Encoding settings #
#####################
# where to log the encoder output
ENC_LOG=/dev/null

# Suggested: --vbr-new -t --nores -S
#            VBR, independent frames, silent mode
LAME_OPTS="--vbr-new -t --nores -S"

# Suggested:
# XXX: suggest a default
SPEEX_OPTS=""

# Suggested: -q0 --downmix
#            Low quality, mono
VORBIS_OPTS="-q0 --downmix"

###################
# End of settings #
###################

# Check if executables exist and perform any necessary initialisation
init_tts() {
    case $TTS_ENGINE in
        festival)
            # Check for festival_client
            if [ "`which $FESTIVAL_CLIENT`" = "" ]; then
                echo "Error: $FESTIVAL_CLIENT not found"
                exit 4
            fi

            # Check for, and start festival server if specified
            if [ X$FESTIVAL_START = XY ]; then
                if [ "`which $FESTIVAL_BIN`" = "" ]; then
                    echo "Error: $FESTIVAL_BIN not found"
                    exit 3
                fi
                FESTIVAL_HOST='localhost'
                $FESTIVAL_BIN --server 2>&1 > /dev/null &
                FESTIVAL_SERVER_PID=$!
                sleep 3
                if [ `ps | grep -c "^\ *$FESTIVAL_SERVER_PID"` -ne 1 ]; then
                    echo "Error: Festival not started"
                    exit 9
                fi
            fi
            # Test connection to festival server
            output=`echo -E "Rockbox" | $FESTIVAL_CLIENT --server \
                   $FESTIVAL_HOST --otype riff --ttw --output \
                   /dev/null 2>&1`
            if [ $? -ne 0 ]; then
                echo "Error: Couldn't connect to festival server at" \
                     "$FESTIVAL_HOST ($output)"
                exit 8
            fi
            ;;
        flite)
            # Check for flite
            if [ "`which $FLITE_BIN`" = "" ]; then
                echo "Error: $FLITE_BIN not found"
                exit 5
            fi
            ;;
        espeak)
            # Check for espeak
            if [ "`which $ESPEAK_BIN`" = "" ]; then
                echo "Error: $ESPEAK_BIN not found"
                exit 5
            fi
            ;;
        sapi5)
            # Check for SAPI5
            cscript /B $SAPI5DIR/sapi5_init_tts.vbs
            if [ $? -ne 0 ]; then
                echo "Error: SAPI 5 not available"
                exit 5
            fi
            ;;
        *)
            echo "Error: no valid TTS engine selected: $TTS_ENGINE"
            exit 2
            ;;
    esac
    if [ ! -x $WAVTRIM ]; then
        echo "Error: $WAVTRIM is not available"
        exit 11
    fi
}

# Perform any necessary shutdown for TTS engine
stop_tts() {
    case $TTS_ENGINE in
        festival)
            if [ X$FESTIVAL_START = XY ]; then
                # XXX: This is probably possible to do using festival_client
                kill $FESTIVAL_SERVER_PID > /dev/null 2>&1
            fi
            ;;
    esac
}

# Check if executables exist and perform any necessary initialisation
init_encoder() {
    case $ENCODER in
        lame)
            # Check for lame binary
            if [ "" = "`which $LAME_BIN`" ]; then
                echo "Error: $LAME_BIN not found"
                exit 6
            fi
            ;;
        speex)
            # Check for speexenc binary
            if [ "" = "`which $SPEEX_BIN`" ]; then
                echo "Error: $SPEEX_BIN not found"
                exit 7
            fi
            ;;
        vorbis)
            # Check for vorbis encoder binary
            if [ "" = "`which $VORBIS_BIN`" ]; then
                echo "Error: $VORBIS_BIN not found"
                exit 10
            fi
            ;;
        *)
            echo "Error: no valid encoder selected: $ENCODER"
            exit 1
            ;;
    esac

}

# Encode file $1 with ENCODER and save the result in $2, delete $1 if specified
encode() {
    INPUT=$1
    OUTPUT=$2

    if [ ! -f "$INPUT" ]; then
        echo "Warning: missing input file: \"$INPUT\""
    else
        echo "Action: Encode   $OUTPUT with $ENCODER"
        case $ENCODER in
            lame)
                $LAME_BIN $LAME_OPTS "$WAV_FILE" "$OUTPUT" >>$ENC_LOG 2>&1
                ;;
            speex)
                $SPEEX_BIN $SPEEX_OPTS "$WAV_FILE" "$OUTPUT" >>$ENC_LOG 2>&1
                ;;
            vorbis)
                $VORBIS_BIN $VORBIS_OPTS "$WAV_FILE" -o "$OUTPUT" >>$ENC_LOG 2>&1
        esac
        if [ ! -f "$OUTPUT" ]; then
            echo "Warning: missing output file \"$OUTPUT\""
        fi
    fi
}

# Generate file $2 containing $1 spoken by TTS_ENGINE, trim silence 
voice() {
    TO_SPEAK=$1
    WAV_FILE=$2
    if [ ! -f "$WAV_FILE" ] || [ X$OVERWRITE_WAV = XY ]; then
        if [ "${TO_SPEAK}" = "" ]; then
            touch "$WAV_FILE"
        else
            case $TTS_ENGINE in
                festival)
                    echo "Action: Generate $WAV_FILE with festival"
                    echo -E "$TO_SPEAK" | $FESTIVAL_CLIENT $FESTIVAL_OPTS \
                         --server $FESTIVAL_HOST \
                         --otype riff --ttw --output "$WAV_FILE" 2>"$WAV_FILE"
                    ;;
                espeak)
                    echo "Action: Generate $WAV_FILE with eSpeak"
                    echo $ESPEAK_BIN $ESPEAK_OPTS -w "$WAV_FILE"
                    echo -E "$TO_SPEAK" | $ESPEAK_BIN $ESPEAK_OPTS -w "$WAV_FILE"
                    ;;
                flite)
                    echo "Action: Generate $WAV_FILE with flite"
                    echo -E "$TO_SPEAK" | $FLITE_BIN $FLITE_OPTS -o "$WAV_FILE"
                    ;;
                sapi5)
                    cscript /B "$SAPI5DIR\sapi5_voice.vbs" ""$TO_SPEAK"" "$WAV_FILE"
                    ;;
            esac
        fi
    fi
    trim "$WAV_FILE"
}

# Trim wavefile $1
trim() {
    WAVEFILE="$1"
    echo "Action: Trim     $WAV_FILE"
    $WAVTRIM "$WAVEFILE" $NOISEFLOOR
}

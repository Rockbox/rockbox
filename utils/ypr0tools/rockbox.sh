######################################################################
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
#   * Samsung YP-R0 Rockbox as an application loader *
#   Lorenzo Miori (C) 2011
######################################################################

# This is expected to be sourced by the shell, which is then
# expected to run $MAINFILE

# Check for menu button being pressed. Return immediately to launch the OF
if [ -e "/dev/r1Button" ]
then
    # running on YP-R1 model (volume up button)
    BTN=$(echo -e -n "\x03")
    VAL=$(dd if=/dev/r1Button bs=4 count=1)
else
    # running on YP-R0 model (menu button)
    BTN=$(echo -e -n "\x07")
    VAL=$(dd if=/dev/r0Btn bs=4 count=1)
fi

if [[ "$VAL" = "$BTN" ]]
then
    return
fi


# Blank-Unblank video to get rid of Samsung BootLogo, but turn off backlight before to hide these things :)
echo -n "0" > /sys/devices/platform/afe.0/bli
echo -n "1" > /sys/class/graphics/fb0/blank
echo -n "0" >> /sys/class/graphics/fb0/blank

amixer sset 'Soft Mute' 0
amixer sset 'Master' 85%
# These are needed only for the R1. TODO: Move all of this into the firmware
if [ -e "/usr/local/bin/r1" ]
then
    amixer cset numid=7,iface=MIXER,name='Master Power witch' 2
    amixer cset numid=6,iface=MIXER,name='Master Handfree Switch' 0
    amixer cset numid=5,iface=MIXER,name='Master Mute' 0
    amixer cset numid=9,iface=MIXER,name='Master samplerate' 44100
    amixer cset numid=2,iface=MIXER,name='Master Volume' 28
    amixer cset numid=1,iface=MIXER,name='PCM PlayBack Switch' 2
    amixer cset numid=8,iface=MIXER,name='FM Mute' 0
    amixer cset numid=4,iface=MIXER,name='Capture FM Switch' 0
    amixer cset numid=3,iface=MIXER,name='Capture Mic Switch' 0
fi

# We set-up various settings for the cpu governor: default are
# Every 1,5 s the kernel evaluates if it's the case to down/up clocking the cpu
echo "ondemand" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo "1" > /sys/devices/system/cpu/cpu0/cpufreq/ondemand/ignore_nice_load
echo "150000" > /sys/devices/system/cpu/cpu0/cpufreq/ondemand/sampling_rate
echo "95" > /sys/devices/system/cpu/cpu0/cpufreq/ondemand/up_threshold

# bind these two to the root so that they're writable
mount --bind /mnt/media0/.rockbox /.rockbox
mount --bind /mnt/media0/Playlists /Playlists

MAINFILE="/mnt/media0/.rockbox/rockbox"
# Attempt to copy the executable in the /tmp directory
# This allows an easier USB Mass Storage Mode to be achieved (file handles)
cp $MAINFILE /tmp/rockbox
if [ $? -eq 0 ]
then
    MAINFILE="/tmp/rockbox"
fi
MAINFILE_ARGV=''
MAINFILE_REDIRECT='>/dev/null 2>&1'

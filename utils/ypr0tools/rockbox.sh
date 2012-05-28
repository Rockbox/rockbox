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
var=$(dd if=/dev/r0Btn bs=4 count=1)
# Here a workaround to detect the byte
var2=$(echo -e -n "\x07")

if [[ "$var" = "$var2" ]]
then
    return
fi


# Blank-Unblank video to get rid of Samsung BootLogo, but turn off backlight before to hide these things :)
echo -n "0" > /sys/devices/platform/afe.0/bli
echo -n "1" > /sys/class/graphics/fb0/blank
echo -n "0" >> /sys/class/graphics/fb0/blank

amixer sset 'Soft Mute' 0
amixer sset 'Master' 85%

# We set-up various settings for the cpu governor: default are
# Every 1,5 s the kernel evaluates if it's the case to down/up clocking the cpu
echo "ondemand" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo "1" > /sys/devices/system/cpu/cpu0/cpufreq/ondemand/ignore_nice_load
echo "150000" > /sys/devices/system/cpu/cpu0/cpufreq/ondemand/sampling_rate
echo "95" > /sys/devices/system/cpu/cpu0/cpufreq/ondemand/up_threshold

# bind these two to the root so that they're writable
mount --bind /mnt/media0/.rockbox /.rockbox
mount --bind /mnt/media0/Playlists /Playlists

# replace Samsung's si470x.ko with our si4709.ko to support fm radio
if [ -e /lib/modules/si4709.ko ]
then
  rmmod /lib/modules/si470x.ko
  insmod /lib/modules/si4709.ko
fi

MAINFILE="/mnt/media0/.rockbox/rockbox"
MAINFILE_ARGV=''
MAINFILE_REDIRECT='>/dev/null 2>&1'

#!/bin/sh
#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
#
#shortcut.desktop generator

# Create a .desktop entry to allow the simulator to be executed in newer shells
# $1 executable
# $2 shortcut name
# $3 icon
create_shortcut() {
    path="$(cd "$(dirname "$1")"; pwd)/"
    execname=$(basename "$1")
    shortname="$2"
    icon="$3"
    shortcut="$path$shortname.desktop"

    echo "Creating shortcut $shortcut"
    echo "path: $path"
    echo "exec: $execname"
    echo "icon: $icon"

    echo "[Desktop Entry]
    Encoding=UTF-8
    Version=1.1
    Type=Application
    Categories=Apps;
    Terminal=false
    Name=$shortname
    Path=$path
    Exec=$path/$execname
    Icon=$icon
    Name[en-US]=$shortname" > $shortcut
}

#./genshortcut.sh ./rockboxui target-sim music-app
create_shortcut $@

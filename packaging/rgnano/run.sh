#!/bin/sh

RBDIR=/mnt/FunKey/rockbox
CFGFILE=$RBDIR/config.cfg
BLPATH=/sys/class/backlight/backlight/brightness

# Install the rockbox folder
if [ ! -d $RBDIR ]; then
  notif set 0 " Installing rockbox..."
  mkdir -p $RBDIR
  cp -r ./install/* $RBDIR
  notif clear
else
  OPKV=$(cat ./install/rockbox-info.txt | grep Version: | cut -d'-' -f2)
  SDV=$(cat $RBDIR/rockbox-info.txt | grep Version: | cut -d'-' -f2)

  if [[ $OPKV -gt  $SDV ]]; then
    notif set 0 " Updating rockbox..."
    cp -r -f -u ./install/* $RBDIR
    notif clear
  fi
fi

# Copy default config
if [ ! -f $CFGFILE ]; then
  mkdir -p $(dirname $CFGFILE)
  cp ./config.cfg $CFGFILE
fi

# Get current volume/brightness -> launch rockbox -> restore previous values
CUR_VOL=$(volume get)
CUR_BL=$(cat $BLPATH)
volume set 100

./rockbox

volume set $CUR_VOL
echo $CUR_BL > $BLPATH

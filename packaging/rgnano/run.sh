#!/bin/sh

RBDIR_OLD=/mnt/FunKey/rockbox
RBDIR=/mnt/FunKey/.rockbox
CFGFILE=$RBDIR/config.cfg
BLPATH=/sys/class/backlight/backlight/brightness

_send_sigusr1()
{
  kill -s USR1 "$rb_pid" 2>/dev/null
}

# Check if the old folder exists and rename it
# TODO: Remove this later, maybe when there's a new stable update?
if [ -d $RBDIR_OLD ]; then
  mv $RBDIR_OLD $RBDIR
fi

# Install or update the rockbox folder
if [ ! -d $RBDIR ]; then
  notif set 0 " Installing rockbox..."
  mkdir -p $RBDIR
  cp -r ./install/* $RBDIR
  notif clear
else
  OPKV=$(cat ./install/rockbox-info.txt | grep Version)
  SDV=$(cat $RBDIR/rockbox-info.txt | grep Version)

  if [[ "$OPKV" != "$SDV" ]]; then
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

# Need to send SIGUSR1 to the rockbox process for instant play support
trap _send_sigusr1 SIGUSR1

./rockbox &

rb_pid=$!
wait "$rb_pid"
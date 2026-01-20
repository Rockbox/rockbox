#!/bin/sh

killall    hiby_player    &>/dev/null
killall -9 hiby_player    &>/dev/null

killall    bootloader.rb    &>/dev/null
killall -9 bootloader.rb    &>/dev/null

/usr/bin/bootloader.rb
sleep 1s
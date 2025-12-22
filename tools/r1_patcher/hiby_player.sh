#!/bin/sh

killall    hiby_player    &>/dev/null
killall -9 hiby_player    &>/dev/null

killall    bootloader.r1    &>/dev/null
killall -9 bootloader.r1    &>/dev/null

/usr/bin/bootloader.r1
sleep 1s
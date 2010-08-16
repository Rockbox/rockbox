#!/bin/sh
# Simple script that converts Android's KEYCODE_* ints to preprocessor #defines

URL="http://android.git.kernel.org/?p=platform/frameworks/base.git;a=blob_plain;f=core/java/android/view/KeyEvent.java;hb=HEAD"

echo "Processing $URL..."
(echo "/* Ripped from $URL */";
 curl $URL | grep "public static final int KEYCODE" | sed 's/^.*public static final int \(KEYCODE_.*\) *= *\([0-9]*\).*$/#define \1 \2/'
) > `dirname $0`/android_keyevents.h

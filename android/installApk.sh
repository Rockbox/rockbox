#!/bin/sh
ADB="$ANDROID_SDK_PATH/tools/adb"
if [ ! -e $ADB ]
then
  # Starting with the gingerbread sdk, the adb location changed
  ADB="$ANDROID_SDK_PATH/platform-tools/adb"
fi

$ADB install -r rockbox.apk
echo 'am start -a android.intent.action.MAIN -n org.rockbox/.RockboxActivity; exit' | $ADB shell

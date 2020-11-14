/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* This config file is for Rockbox as an application on Android without JVM. */

/* We don't run on hardware directly */
#ifndef SIMULATOR
#define CONFIG_PLATFORM PLATFORM_HOSTED
#define PIVOT_ROOT "/mnt/sdcard"
#endif
#define HAVE_FPU

/* For Rolo and boot loader */
#define MODEL_NUMBER 94

#define MODEL_NAME "iBasso DX50"

#define USB_NONE
#define HAVE_USB_POWER

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  320
#define LCD_HEIGHT 240
#define LCD_DPI 166
#define LCD_DEPTH  16
#define LCD_PIXELFORMAT RGB565

#define HAVE_LCD_ENABLE
#define HAVE_LCD_SLEEP
#define HAVE_LCD_SLEEP_SETTING
/*#define HAVE_LCD_FLIP*/
#define HAVE_LCD_SHUTDOWN

/* define this to indicate your device's keypad */
#define HAVE_TOUCHSCREEN
#define HAVE_BUTTON_DATA
#define HAS_BUTTON_HOLD

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x160000

#define AB_REPEAT_ENABLE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      4
#define MAX_BRIGHTNESS_SETTING      255
#define DEFAULT_BRIGHTNESS_SETTING  255

/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING



#define HAVE_SW_TONE_CONTROLS
#define HAVE_SW_VOLUME_CONTROL
#define HW_SAMPR_CAPS SAMPR_CAP_ALL_96

//#define HAVE_MULTIMEDIA_KEYS
#define CONFIG_KEYPAD DX50_PAD

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

#define BATTERY_CAPACITY_DEFAULT 2100 /* default battery capacity */
#define BATTERY_CAPACITY_MIN     1700 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX     3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC       50 /* capacity increment */
#define BATTERY_TYPES_COUNT         1 /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE
#define CONFIG_CHARGING        CHARGING_MONITOR

/*
    10 hours from a 2100 mAh battery
    Based on battery bench with stock Samsung battery.
*/
#define CURRENT_NORMAL    210
#define CURRENT_BACKLIGHT  30 /* TBD */
#define CURRENT_RECORD      0 /* no recording */

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define CONFIG_LCD LCD_COWOND2

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

/* No special storage */
#define CONFIG_STORAGE STORAGE_HOSTFS
#define HAVE_STORAGE_FLUSH

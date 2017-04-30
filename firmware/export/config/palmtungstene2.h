/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2015 by Lorenzo Miori: Rockbox port to Palm Tungsten E2
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
#define CONFIG_PLATFORM PLATFORM_HOSTED

/* For Rolo and boot loader */
#define MODEL_NUMBER 96

#define MODEL_NAME "Palm Tungsten E2"

#define USB_NONE

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

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
#define LCD_WIDTH       320
#define LCD_HEIGHT      320
#define LCD_DPI         143
#define LCD_DEPTH       16
#define LCD_PIXELFORMAT RGB565

#define HAVE_LCD_ENABLE
//#define HAVE_LCD_SLEEP
//#define HAVE_LCD_SLEEP_SETTING
#define HAVE_LCD_SHUTDOWN

/* define this to indicate your device's keypad */
//#define HAVE_TOUCHSCREEN
//#define HAVE_BUTTON_DATA

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x100000

#define AB_REPEAT_ENABLE
#define ACTION_WPSAB_SINGLE ACTION_WPS_HOTKEY

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      0x16
#define MAX_BRIGHTNESS_SETTING      0xFE
#define DEFAULT_BRIGHTNESS_SETTING  0x7F

/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC
#define HAVE_SW_TONE_CONTROLS
#define HAVE_SW_VOLUME_CONTROL
#define HW_SAMPR_CAPS SAMPR_CAP_ALL
#define HAVE_PLAY_FREQ

//#define HAVE_DUMMY_CODEC

#define CONFIG_KEYPAD PALM_TUNGSTENE2_PAD

#define BATTERY_CAPACITY_DEFAULT 1020 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1020  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 1020 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE
#define CONFIG_CHARGING        CHARGING_MONITOR

#define CURRENT_NORMAL    210 /* TBD */
#define CURRENT_BACKLIGHT  30 /* TBD */
#define CURRENT_RECORD      0 /* no recording */

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define CONFIG_LCD LCD_PALMTUNGSTENE2

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

/* No special storage */
#define CONFIG_STORAGE STORAGE_HOSTFS
#define HAVE_STORAGE_FLUSH

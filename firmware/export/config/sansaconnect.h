/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2011 by Tomasz Moń
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

/*
 * This config file is for the Sansa Connect
 */

/* This is the absolute address on the bus set by OF bootloader */
#define CONFIG_SDRAM_START 0x01000000

#define SANSA_CONNECT 1
#define MODEL_NAME   "Sandisk Sansa Connect"

/* For Rolo and boot loader */
#define MODEL_NUMBER 81

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

/* define this if you use an SD controller */
#define CONFIG_STORAGE STORAGE_SD

#define HAVE_MULTIDRIVE
#define NUM_DRIVES 2
#define HAVE_HOTSWAP
#define HAVE_HOTSWAP_STORAGE_AS_MAIN

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

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* define this if you want viewport clipping enabled for safe LCD functions */
#define HAVE_VIEWPORT_CLIP

/* LCD dimensions */
#define CONFIG_LCD LCD_CONNECT

#define LCD_WIDTH  240
#define LCD_HEIGHT 320

#define LCD_DEPTH  16   /* 65k colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

#define HAVE_LCD_ENABLE
#ifndef BOOTLOADER
#define HAVE_LCD_SLEEP
#endif

#define LCD_SLEEP_TIMEOUT (2*HZ)

#define MAX_ICON_HEIGHT 35
#define MAX_ICON_WIDTH 35


#define CONFIG_KEYPAD SANSA_CONNECT_PAD

/* Define this to have CPU bootsted while scrolling in the UI */
#define HAVE_GUI_BOOST

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

#define HAVE_MORSE_INPUT

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

//#define HAVE_HARDWARE_BEEP

/* There is no hardware tone control */
#define HAVE_SW_TONE_CONTROLS

#define HAVE_AIC3X

//#define HW_SAMPR_CAPS SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11 | SAMPR_CAP_8

/* define this if you have a real-time clock */
//#define CONFIG_RTC RTC_STM41T62

/* define this if the unit uses a scrollwheel for navigation */
#define HAVE_SCROLLWHEEL

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#define HAVE_BACKLIGHT_BRIGHTNESS

#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING          1
#define MAX_BRIGHTNESS_SETTING          20
#define DEFAULT_BRIGHTNESS_SETTING      16 /* OF default brightness (80%) */
#define DEFAULT_DIMNESS_SETTING         6  /* OF default inactive (30%) */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x200000

#define BATTERY_CAPACITY_DEFAULT 800 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 700     /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 1000    /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 100     /* capacity increment */
#define BATTERY_TYPES_COUNT  1       /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* define current usage levels */
#if 0
/* TODO */
#define CURRENT_NORMAL     85
#define CURRENT_BACKLIGHT  200
#endif

/* Hardware controlled charging with monitoring */
//#define CONFIG_CHARGING CHARGING_MONITOR

#define CONFIG_CPU DM320

#define CONFIG_I2C I2C_DM320
#define HAVE_SOFTWARE_I2C

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ 150000000

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#if 0
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0x0781
#define USB_PRODUCT_ID 0x7480
#endif

#define INCLUDE_TIMEOUT_API

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "sansa"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
 * This config file is for the Creative Zen Vision:M
 */
#define TARGET_TREE /* this target is using the target tree system */

#define CONFIG_SDRAM_START 0x00900000

#define CREATIVE_ZVM 1

/* This makes it part of the Creative Zen Vision family :) */
#define CREATIVE_ZVx 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 26

/* define this if you use an ATA controller */
#define HAVE_ATA

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if the target has volume keys which can be used in the lists */
//#define HAVE_VOLUME_IN_LIST

/* LCD dimensions */
#define CONFIG_LCD LCD_CREATIVEZVM

/* choose the lcd orientation. both work */
#define CONFIG_ORIENTATION SCREEN_PORTRAIT

#define LCD_WIDTH  320
#define LCD_HEIGHT 240

#define LCD_DEPTH  16   /* 65k colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE

#define CONFIG_KEYPAD CREATIVEZVM_PAD
#define HAS_BUTTON_HOLD
#define HAVE_HEADPHONE_DETECTION
//#define HAVE_TOUCHPAD

#define CONFIG_I2C  I2C_DM320

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
//#define CONFIG_RTC RTC_RX5X348AB

/* Define this for LCD backlight available */
//#define HAVE_BACKLIGHT

//#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
//#define MIN_BRIGHTNESS_SETTING          0
//#define MAX_BRIGHTNESS_SETTING          127
//#define DEFAULT_BRIGHTNESS_SETTING      85 /* OF "full brightness" */
//#define DEFAULT_DIMNESS_SETTING         22 /* OF "most dim" */

/* Define this if you have a software controlled poweroff */
//#define HAVE_SW_POWEROFF
//TODO: enable this back

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x100000

/* Define this if you have the TLV320 audio codec -> controlled by the DSP */
#define HAVE_TLV320

#define CONFIG_I2C I2C_DM320

/* TLV320 has no tone controls, so we use the software ones */
#define HAVE_SW_TONE_CONTROLS

/*#define HW_SAMPR_CAPS (SAMPR_CAP_88 | SAMPR_CAP_44 | SAMPR_CAP_22 | \
    SAMPR_CAP_11)*/

#define BATTERY_CAPACITY_DEFAULT 1250 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 500        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2500        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 100         /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

/* Hardware controlled charging with monitoring */
//#define CONFIG_CHARGING CHARGING_MONITOR

#ifndef SIMULATOR

/* Define this if you have a TI TMS320DM320 */
#define CONFIG_CPU DM320

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The size of the flash ROM */
#define FLASH_SIZE 0x400000

/* Define this to the CPU frequency */
#define CPU_FREQ 16934400

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* define this if the backlight can be set to a brightness */
//#define __BACKLIGHT_INIT

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* Define this if you have adjustable CPU frequency */
/* #define HAVE_ADJUSTABLE_CPU_FREQ */

#define BOOTFILE_EXT "zvm"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define CONFIG_USBOTG	USBOTG_ISP1583
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0x041e 
#define USB_PRODUCT_ID 0x4133

/*DEBUGGING!*/
#ifdef BOOTLOADER
#define THREAD_EXTRA_CHECKS 1
#define DEBUG 1
#define debug(msg) printf(msg)
#define BUTTON_DEBUG
#endif

#endif

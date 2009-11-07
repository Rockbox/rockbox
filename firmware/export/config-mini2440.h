/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2009 by Bob Cousins, Lyre Project
 * Copyright (C) 2009 by Jorge Pinto, Lyre Project
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
 * This config file is for the Mini2440
 */
#define TARGET_TREE /* this target is using the target tree system */

/* For Rolo and boot loader */
#define MODEL_NUMBER 131
#define MODEL_NAME "Mini2440"

/***************************************************************************/
/* Hardware Config */

/* TODO: ??? */
#define CONFIG_SDRAM_START 0x30000000 

/* Flash storage */
#define HAVE_FLASH_STORAGE
/* define the storage type */
#define CONFIG_STORAGE STORAGE_SD

/*
#define HAVE_MULTIDRIVE
#define NUM_DRIVES 2
#define HAVE_HOTSWAP
*/

/* Disk storage */
/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
/* #define HAVE_DISK_STORAGE */

/* Display */
/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP
/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR
/* The LCD is assumed to be 3.5" TFT touch screen, others are possible */
#define CONFIG_LCD LCD_MINI2440
/* LCD dimensions */
#define LCD_WIDTH  240
#define LCD_HEIGHT 320
/* The LCD is configured for RGB565 */
#define LCD_DEPTH  16          /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */
/* Define this for LCD backlight available */
/* The Mini2440 supports backlight brightness depending on LCD type */
/* But the 3.5" LCD touch screen does not support brightness*/
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING          1   /* 0.5 mA */
#define MAX_BRIGHTNESS_SETTING          12  /* 32 mA */
#define DEFAULT_BRIGHTNESS_SETTING      10  /* 16 mA */

/* Keypad */
#define CONFIG_KEYPAD MINI2440_PAD
#define HAVE_TOUCHSCREEN
#define HAVE_BUTTON_DATA

/* I2C */
/* We do not use currently use hardware I2C, but does not build without */
#define CONFIG_I2C I2C_S3C2440

/* Define DAC/Codec */
#define HAVE_UDA1341

#define HW_SAMPR_CAPS (SAMPR_CAP_44 | SAMPR_CAP_22 | SAMPR_CAP_11)

/* Battery */
#define BATTERY_CAPACITY_DEFAULT 1100   /* default battery capacity */
#define BATTERY_CAPACITY_MIN      500   /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX     2500   /* max. capacity selectable */
#define BATTERY_CAPACITY_INC      100   /* capacity increment */
#define BATTERY_TYPES_COUNT         1   /* only one type */



/***************************************************************************/
/* Application Config */

#define HAVE_ALBUMART
/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING
/* define this to enable JPEG decoding */
#define HAVE_JPEG
/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN
#define HAVE_QUICKSCREEN

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_S3C2440

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x100000

#define CONFIG_CPU S3C2440

/* Define this to the CPU frequency */
#define CPU_FREQ      406000000
#define MCK_FREQ   (CPU_FREQ/4)
#define SLOW_CLOCK        32768


/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST
#define MIN_CONTRAST_SETTING        0
#define MAX_CONTRAST_SETTING        63
#define DEFAULT_CONTRAST_SETTING    47 /* Match boot contrast */

/* USB */
/* TODO:#define HAVE_USBSTACK */
#define USB_NONE

#define HAVE_SERIAL

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR
/*#define POWER_INPUT_BATTERY 0*/

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define BOOTFILE_EXT "mini2440"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"


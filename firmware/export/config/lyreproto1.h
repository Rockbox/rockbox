/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 *
 * Copyright (C) 2009 by Jorge Pinto
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
 * This config file is for the Lyre prototype 1.
 */
#define TARGET_TREE /* this target is using the target tree system */

#define CONFIG_SDRAM_START 0x20000000

/* For Rolo and boot loader */
#define MODEL_NUMBER 130

/* define this if the flash memory uses the
 * SecureDigital Memory Card protocol */
#define CONFIG_STORAGE STORAGE_SD
#define HAVE_FLASH_STORAGE

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

#define CONFIG_LCD LCD_LYRE_PROTO1

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 128
/* The LCD used is just rgb444, 64 colours. We do a bit conversion on LCD
 * drivers. */
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

#define CONFIG_KEYPAD LYRE_PROTO1_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x100000

/* Lyre prototype 1 do not use I2C, just SPI */
#define CONFIG_I2C I2C_NONE

/* Define this if you have the TLV320 audio codec -> controlled by the DSP */
#define HAVE_TLV320

/* TLV320 has no tone controls, so we use the software ones */
#define HAVE_SW_TONE_CONTROLS

#define BATTERY_CAPACITY_DEFAULT 1100 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 500        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2500        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 100         /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

#define CONFIG_CPU AT91SAM9260

/* Define this to the CPU frequency */
#define CPU_FREQ 198656000
#define MCK_FREQ 99328000
#define SLOW_CLOCK 32768

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define BOOTFILE_EXT "lyre_proto1"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"


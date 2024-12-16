/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright (C) 2017 Cástor Muñoz
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
#include <stdint.h>
#include "config.h"

#include "lcd-s5l8702.h"
#ifdef BOOTLOADER
#include "piezo.h"
#endif


// XXX: For nano3g displays see ili9327, which says it supports 8-bit MIDI interface command and 8-bit param

/* Supported LCD types */
enum {
    LCD_TYPE_UNKNOWN = -1,
    LCD_TYPE_38B3 = 0,
    LCD_TYPE_38C4,
    LCD_TYPE_38D5,
    LCD_TYPE_38E6,
    LCD_TYPE_58XX,
    N_LCD_TYPES
};

#if defined(HAVE_LCD_SLEEP) || defined(HAVE_LCD_SHUTDOWN)
/* sleep sequences */

/* 0xb3, 0xe6 */
static const uint8_t lcd_sleep_seq_03[] =
{
    CMD,   0x28,  0,    /* Display Off */
    CMD,   0x10,  0,    /* Sleep In */
    SLEEP, 5,           /* 50 ms */
    END
};

/* 0xc4 */
static const uint8_t lcd_sleep_seq_1[] =
{
    CMD,   0x28,  0,    /* Display Off */
    CMD,   0x10,  0,    /* Sleep In */
    SLEEP, 12,          /* 120 ms */
    END
};

/* 0xd5 */
static const uint8_t lcd_sleep_seq_2[] =
{
    CMD,   0x28,  0,    /* Display Off */
    CMD,   0x10,  0,    /* Sleep In */
    END
};

/* 0x58 */
static const uint8_t lcd_sleep_seq_4[] =
{
    CMD,   0x10,  0,    /* Sleep In */
    END
};

// static const void* seq_sleep_by_type[] =
static void* seq_sleep_by_type[] =
{
    [LCD_TYPE_38B3] = (void*) lcd_sleep_seq_03,
    [LCD_TYPE_38C4] = (void*) lcd_sleep_seq_1,
    [LCD_TYPE_38D5] = (void*) lcd_sleep_seq_2,
    [LCD_TYPE_38E6] = (void*) lcd_sleep_seq_03,
    [LCD_TYPE_58XX] = (void*) lcd_sleep_seq_4,
};
#endif /* HAVE_LCD_SLEEP || HAVE_LCD_SHUTDOWN */

#if defined(HAVE_LCD_SLEEP)
/* awake sequences */

/* 0xb3, 0xc4, 0xd5, 0xd6, 0x58 */
static const uint8_t lcd_awake_seq_01234[] =
{
    CMD,   0x11,  0,    /* Sleep Out */
    SLEEP, 12,          /* 120 ms */
    CMD,   0x29,  0,    /* Display On */
    SLEEP, 1,           /* 10 ms */
    END
};

static void* seq_awake_by_type[] =
{
    [LCD_TYPE_38B3] = (void*) lcd_awake_seq_01234,
    [LCD_TYPE_38C4] = (void*) lcd_awake_seq_01234,
    [LCD_TYPE_38D5] = (void*) lcd_awake_seq_01234,
    [LCD_TYPE_38E6] = (void*) lcd_awake_seq_01234,
    [LCD_TYPE_58XX] = (void*) lcd_awake_seq_01234,
};
#endif /* HAVE_LCD_SLEEP */

#if defined(BOOTLOADER)
/* init sequences */

// TODO: put something else at the end of the init sequence???, the bootloader without a sleep is very tight ()

/* 0xb3 */
static const uint8_t lcd_init_seq_0[] =
{
    CMD,   0xef,  1, 0x80,

    /* Power control */
    CMD,   0xc0,  1, 0x06,
    CMD,   0xc1,  1, 0x03,
    CMD,   0xc2,  2, 0x12, 0x00,
    CMD,   0xc3,  2, 0x12, 0x00,
    CMD,   0xc4,  2, 0x12, 0x00,
    CMD,   0xc5,  2, 0x40, 0x38,

    /* Display control */
    CMD,   0xb1,  2, 0x5f, 0x3f,
    CMD,   0xb2,  2, 0x5f, 0x3f,
    CMD,   0xb3,  2, 0x5f, 0x3f,
    CMD,   0xb4,  1, 0x02,
    CMD,   0xb6,  2, 0x12, 0x02,

    CMD,   0x35,  1, 0x00,  /* Tearing Effect Line On */
    CMD,   0x26,  1, 0x10,  /* Gamma Set */

    CMD,   0xfe,  1, 0x00,

    /* Gamma settings */
    CMD,   0xe0, 11, 0x0f, 0x70, 0x47, 0x03, 0x02, 0x02, 0xa0, 0x94,
                     0x05, 0x00, 0x0e,
    CMD,   0xe1, 11, 0x02, 0x43, 0x77, 0x00, 0x0f, 0x05, 0x49, 0x0a,
                     0x02, 0x0e, 0x00,
    CMD,   0xe2, 11, 0x2f, 0x63, 0x20, 0x50, 0x00, 0x07, 0xd1, 0x13,
                     0x00, 0x00, 0x0e,
    CMD,   0xe3, 11, 0x50, 0x20, 0x60, 0x23, 0x0f, 0x00, 0x31, 0x1d,
                     0x07, 0x0e, 0x00,
    CMD,   0xe4, 11, 0x5e, 0x50, 0x65, 0x27, 0x00, 0x0b, 0xdf, 0xf1,
                     0x01, 0x00, 0x0e,
    CMD,   0xe5, 11, 0x20, 0x67, 0x55, 0x50, 0x0e, 0x01, 0x1f, 0xfd,
                     0x0b, 0x0e, 0x00,

    CMD,   0x3a,  1, 0x06,  /* Pixel Format Set */
    CMD,   0x36,  1, 0x60,  /* Memory Access Control */
    CMD,   0x13,  0,        /* Normal Mode On */

    /* Awake sequence */
    CMD,   0x11,  0,    /* Sleep Out */
    SLEEP, 12,          /* 120 ms */
    CMD,   0x29,  0,    /* Display On */
    SLEEP, 1,           /* 10 ms */
    END
};

/* 0xc4 */
static const uint8_t lcd_init_seq_1[] =
{
    CMD,   0x01,  0,    /* Software Reset */
    SLEEP, 1,           /* 10 ms */

    /* Power control */
    CMD,   0xc0,  1, 0x01,
    CMD,   0xc1,  1, 0x03,
    CMD,   0xc2,  2, 0x74, 0x00,
    CMD,   0xc3,  2, 0x72, 0x03,
    CMD,   0xc4,  2, 0x73, 0x03,
    CMD,   0xc5,  2, 0x3c, 0x3c,

    /* Display control */
    CMD,   0xb1,  2, 0x6a, 0x15,
    CMD,   0xb2,  2, 0x6a, 0x15,
    CMD,   0xb3,  2, 0x6a, 0x15,
    CMD,   0xb4,  1, 0x02,
    CMD,   0xb6,  2, 0x12, 0x02,

    CMD,   0x35,  1, 0x00,  /* Tearing Effect Line On */
    CMD,   0x26,  1, 0x10,  /* Gamma Set */

    /* Gamma settings */
    CMD,   0xe0, 11, 0x1e, 0x22, 0x44, 0x00, 0x09, 0x01, 0x47, 0xc1,
                     0x05, 0x02, 0x09,
    CMD,   0xe1, 11, 0x0f, 0x32, 0x35, 0x00, 0x03, 0x05, 0x5e, 0x78,
                     0x03, 0x00, 0x03,
    CMD,   0xe2, 11, 0x0d, 0x74, 0x47, 0x41, 0x07, 0x01, 0x74, 0x41,
                     0x09, 0x03, 0x07,
    CMD,   0xe3, 11, 0x5f, 0x41, 0x27, 0x02, 0x00, 0x03, 0x43, 0x55,
                     0x02, 0x00, 0x03,
    CMD,   0xe4, 11, 0x1b, 0x53, 0x44, 0x51, 0x0b, 0x01, 0x64, 0x20,
                     0x05, 0x02, 0x09,
    CMD,   0xe5, 11, 0x7f, 0x41, 0x26, 0x02, 0x04, 0x00, 0x33, 0x35,
                     0x01, 0x00, 0x02,

    CMD,   0x3a,  1, 0x66,  /* Pixel Format Set */
    CMD,   0x36,  1, 0x60,  /* Memory Access Control */

    /* Awake sequence */
    CMD,   0x11,  0,    /* Sleep Out */
    SLEEP, 12,          /* 120 ms */
    CMD,   0x29,  0,    /* Display On */
    SLEEP, 1,           /* 10 ms */
    END
};

/* 0xd5 */
static const uint8_t lcd_init_seq_2[] =
{
    CMD,   0xfe,  1, 0x00,

    /* Power control */
    CMD,   0xc0,  1, 0x00,
    CMD,   0xc1,  1, 0x03,
    CMD,   0xc2,  2, 0x73, 0x03,
    CMD,   0xc3,  2, 0x73, 0x03,
    CMD,   0xc4,  2, 0x73, 0x03,
    CMD,   0xc5,  2, 0x64, 0x37,

    /* Display control */
    CMD,   0xb1,  2, 0x69, 0x13,
    CMD,   0xb2,  2, 0x69, 0x13,
    CMD,   0xb3,  2, 0x69, 0x13,
    CMD,   0xb4,  1, 0x02,
    CMD,   0xb6,  2, 0x03, 0x12,

    CMD,   0x35,  1, 0x00,  /* Tearing Effect Line On */
    CMD,   0x26,  1, 0x10,  /* Gamma Set */

    /* Gamma settings */
    CMD,   0xe0, 11, 0x08, 0x00, 0x10, 0x00, 0x03, 0x0e, 0xc8, 0x65,
                     0x05, 0x00, 0x00,
    CMD,   0xe1, 11, 0x06, 0x20, 0x00, 0x00, 0x00, 0x07, 0x4d, 0x0b,
                     0x08, 0x00, 0x00,
    CMD,   0xe2, 11, 0x08, 0x77, 0x27, 0x63, 0x0f, 0x16, 0xcf, 0x25,
                     0x03, 0x00, 0x00,
    CMD,   0xe3, 11, 0x5f, 0x53, 0x77, 0x06, 0x00, 0x02, 0x4b, 0x7b,
                     0x0f, 0x00, 0x00,
    CMD,   0xe4, 11, 0x08, 0x46, 0x57, 0x52, 0x0f, 0x16, 0xcf, 0x25,
                     0x04, 0x00, 0x00,
    CMD,   0xe5, 11, 0x6f, 0x42, 0x57, 0x06, 0x00, 0x04, 0x43, 0x7b,
                     0x0f, 0x00, 0x00,

    CMD,   0x3a,  1, 0x66,  /* Pixel Format Set */
    CMD,   0x36,  1, 0x60,  /* Memory Access Control */

    /* Awake sequence */
    CMD,   0x11,  0,    /* Sleep Out */
    SLEEP, 12,          /* 120 ms */
    CMD,   0x29,  0,    /* Display On */
    SLEEP, 1,           /* 10 ms */
    END
};

/* 0xe6 */
static const uint8_t lcd_init_seq_3[] =
{
    CMD,   0xef,  1, 0x80,

    /* Power control */
    CMD,   0xc0,  1, 0x0a,
    CMD,   0xc1,  1, 0x03,
    CMD,   0xc2,  2, 0x12, 0x00,
    CMD,   0xc3,  2, 0x12, 0x00,
    CMD,   0xc4,  2, 0x12, 0x00,
    CMD,   0xc5,  2, 0x38, 0x38,

    /* Display control */
    CMD,   0xb1,  2, 0x5f, 0x3f,
    CMD,   0xb2,  2, 0x5f, 0x3f,
    CMD,   0xb3,  2, 0x5f, 0x3f,
    CMD,   0xb4,  1, 0x02,
    CMD,   0xb6,  2, 0x12, 0x02,

    CMD,   0x35,  1, 0x00,  /* Tearing Effect Line On */
    CMD,   0x26,  1, 0x10,  /* Gamma Set */

    CMD,   0xfe,  1, 0x00,

    /* Gamma settings */
    CMD,   0xe0, 11, 0x0f, 0x70, 0x47, 0x03, 0x02, 0x02, 0xa0, 0x94,
                     0x05, 0x00, 0x0e,
    CMD,   0xe1, 11, 0x02, 0x43, 0x77, 0x00, 0x0f, 0x05, 0x49, 0x0a,
                     0x02, 0x0e, 0x00,
    CMD,   0xe2, 11, 0x2f, 0x63, 0x20, 0x50, 0x00, 0x07, 0xd1, 0x13,
                     0x00, 0x00, 0x0e,
    CMD,   0xe3, 11, 0x50, 0x20, 0x60, 0x23, 0x0f, 0x00, 0x31, 0x1d,
                     0x07, 0x0e, 0x00,
    CMD,   0xe4, 11, 0x5e, 0x50, 0x65, 0x27, 0x00, 0x0b, 0xdf, 0xf1,
                     0x01, 0x00, 0x0e,
    CMD,   0xe5, 11, 0x20, 0x67, 0x55, 0x50, 0x0e, 0x01, 0x1f, 0xfd,
                     0x0b, 0x0e, 0x00,

    CMD,   0x3a,  1, 0x06,  /* Pixel Format Set */
    CMD,   0x36,  1, 0x60,  /* Memory Control Access */
    CMD,   0x13,  0,        /* Normal Mode On */

    /* Awake sequence */
    CMD,   0x11,  0,    /* Sleep Out */
    SLEEP, 12,          /* 120 ms */
    CMD,   0x29,  0,    /* Display On */
    SLEEP, 1,           /* 10 ms */
    END
};

/* 0x58 */
static const uint8_t lcd_init_seq_4[] =
{
    CMD,   0xe1,  3, 0x0f, 0x31, 0x04,
    CMD,   0xe2,  5, 0x02, 0xa2, 0x08, 0x11, 0x01,
    CMD,   0xe3,  2, 0x10, 0x88,
    CMD,   0xe4,  2, 0x10, 0x88,
    CMD,   0xe5,  2, 0x00, 0x88,
    CMD,   0xe7, 13, 0x33, 0x0d, 0x57, 0x0e, 0x57, 0x05, 0x57, 0x02,
                     0x04, 0x10, 0x0b, 0x02, 0x02,
    CMD,   0xe8,  5, 0x30, 0x0d, 0x84, 0x8c, 0x21,
    CMD,   0xe9,  5, 0x4b, 0x1a, 0xba, 0x60, 0x11,
    CMD,   0xea,  5, 0x4b, 0xba, 0xba, 0x10, 0x11,
    CMD,   0xeb,  5, 0x0b, 0x3a, 0xd9, 0x60, 0x11,
    CMD,   0xef,  3, 0x00, 0x00, 0x00,
    CMD,   0xf0, 18, 0x12, 0x01, 0x21, 0xa5, 0x6c, 0x23, 0x02, 0x01,
                     0x08, 0x0d, 0x1e, 0xde, 0x5a, 0x93, 0xdc, 0x0d,
                     0x1e, 0x17,
    CMD,   0xf1, 18, 0x12, 0x05, 0x21, 0xb5, 0x8d, 0x24, 0x02, 0x01,
                     0x08, 0x0d, 0x1a, 0xde, 0x4a, 0x72, 0xdb, 0x0d,
                     0x1e, 0x17,
    CMD,   0xf2, 18, 0x0e, 0x00, 0x30, 0xd6, 0x8f, 0x34, 0x03, 0x07,
                     0x08, 0x11, 0x1f, 0xcf, 0x29, 0x70, 0xcb, 0x0c,
                     0x18, 0x17,
    CMD,   0xfa,  3, 0x02, 0x00, 0x02,
    CMD,   0xf7,  2, 0x00, 0xc0,

    CMD,   0x35,  1, 0x00,  /* Tearing Effect Line On */
    CMD,   0x36,  1, 0x20,  /* Memory Access Control */

    /* Awake sequence */
    CMD,   0x11,  0,    /* Sleep Out */
    SLEEP, 12,          /* 120 ms */
    CMD,   0x29,  0,    /* Display On */
    SLEEP, 1,           /* 10 ms */
    END
};

static void* seq_init_by_type[] =
{
    [LCD_TYPE_38B3] = (void*) lcd_init_seq_0,
    [LCD_TYPE_38C4] = (void*) lcd_init_seq_1,
    [LCD_TYPE_38D5] = (void*) lcd_init_seq_2,
    [LCD_TYPE_38E6] = (void*) lcd_init_seq_3,
    [LCD_TYPE_58XX] = (void*) lcd_init_seq_4,
};
#endif /* BOOTLOADER */

static struct lcd_info_rec lcd_info = {
    .mpuiface = LCD_MPUIFACE_PAR9,
};

uint8_t lcd_id[4]; // XXX: DEBUG

struct lcd_info_rec* lcd_target_get_info(void)
{
    // uint8_t lcd_id[4];
    int type = LCD_TYPE_UNKNOWN;
    int retry = 3;

    while (retry--)
    {
        lcd_read_display_id(LCD_MPUIFACE_PAR9, &lcd_id[0]);         // TODO?: MPUIFACE_PAR9

        if (lcd_id[1] == 0x58)
        {
            type = LCD_TYPE_58XX;
        }
        else if (lcd_id[1] == 0x38)
        {
            if      (lcd_id[2] == 0xb3) type = LCD_TYPE_38B3;
            else if (lcd_id[2] == 0xc4) type = LCD_TYPE_38C4;
            else if (lcd_id[2] == 0xd5) type = LCD_TYPE_38D5;
            else if (lcd_id[2] == 0xe6) type = LCD_TYPE_38E6;
        }

        if (type != LCD_TYPE_UNKNOWN)
        {
            lcd_info.lcd_type = type;
            //lcd_info.mpuiface = LCD_MPUIFACE_PAR9;
#if defined(HAVE_LCD_SLEEP) || defined(HAVE_LCD_SHUTDOWN)
            lcd_info.seq_sleep = seq_sleep_by_type[type];
#endif
#ifdef HAVE_LCD_SLEEP
            lcd_info.seq_awake = seq_awake_by_type[type];
#endif
#ifdef BOOTLOADER
            lcd_info.seq_init = seq_init_by_type[type];
#endif
            return &lcd_info;
        }
    }

#ifdef BOOTLOADER
    while (1) {
        uint16_t fatal[] = { 3000,500,500, 0 };
        piezo_seq(fatal);
    }
#else
    /* should not happen */
    while (1);    // TODO?: what to do? poweroff?
#endif
}

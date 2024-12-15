/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: lcd-nano2g.c 28868 2010-12-21 06:59:17Z Buschel $
 *
 * Copyright (C) 2009 by Dave Chapman
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
#include <stdlib.h>
#include <stdint.h>

#include "config.h"

#include "s5l87xx.h"
#include "lcd-s5l8702.h"


#if defined(HAVE_LCD_SLEEP) || defined(HAVE_LCD_SHUTDOWN)
/* powersave sequences */

static const uint8_t lcd_sleep_seq_01[] =
{
    CMD,   0x28,  0,    /* Display Off */
    SLEEP, 5,           /* 50 ms */
    CMD,   0x10,  0,    /* Sleep In */
    SLEEP, 5,           /* 50 ms */
    END
};

static const uint16_t lcd_deepstandby_seq_23[] =
{
    /* Display Off */
    MREG16(1),  0x007, 0x0172,
    MREG16(1),  0x030, 0x03ff,
    SLEEP16(9),
    MREG16(1),  0x007, 0x0120,
    MREG16(1),  0x030, 0x0000,
    MREG16(1),  0x100, 0x0780,
    MREG16(1),  0x007, 0x0000,

    /* power supply off */
    MREG16(1),  0x101, 0x0260,
    MREG16(1),  0x102, 0x00a9,
    SLEEP16(3),
    MREG16(1),  0x100, 0x0700,

    /* Deep Standby Mode */
    MREG16(1),  0x100, 0x0704,
    SLEEP16(5),
    END
};
#endif /* HAVE_LCD_SLEEP || HAVE_LCD_SHUTDOWN */

#ifdef HAVE_LCD_SLEEP
/* awake sequences */

static const uint8_t lcd_awake_seq_01[] =
{
    CMD,   0x11,  0,    /* Sleep Out */
    SLEEP, 6,           /* 60 ms */
    CMD,   0x29,  0,    /* Display On */
    END
};
#endif

// XXX: ili9340 DS: pag.168 B7H -> Entry Mode Set, DSTB -> enter this mode and tell how to exit

#if defined(HAVE_LCD_SLEEP) || defined(BOOTLOADER)
/* init sequences */

static const uint16_t lcd_init_seq_23[] =
{
#ifdef HAVE_LCD_SLEEP
    /* release from deep standby mode (ili9320 DS s12.3) */
    CMD16,      0x000,  /* NOP */
    DELAY16(20),        /* 1024 usecs */
    CMD16,      0x000,
    DELAY16(20),
    CMD16,      0x000,
    DELAY16(20),
    CMD16,      0x000,
    DELAY16(20),
    CMD16,      0x000,
    DELAY16(20),
    CMD16,      0x000,
    DELAY16(20),
#endif

    /* Display settings */
    MREG16(1),  0x008, 0x0808,
    MREG16(7),  0x010, 0x0013, 0x0300, 0x0101, 0x0a03, 0x0a0e, 0x0a19, 0x2402,
    MREG16(1),  0x018, 0x0001,
    MREG16(1),  0x090, 0x0021,

    /* Gamma settings */
    MREG16(14), 0x300, 0x0307, 0x0003, 0x0402, 0x0303, 0x0300, 0x0407, 0x1c04,
                       0x0307, 0x0003, 0x0402, 0x0303, 0x0300, 0x0407, 0x1c04,
    MREG16(14), 0x310, 0x0707, 0x0407, 0x0306, 0x0303, 0x0300, 0x0407, 0x1c01,
                       0x0707, 0x0407, 0x0306, 0x0303, 0x0300, 0x0407, 0x1c01,
    MREG16(14), 0x320, 0x0206, 0x0102, 0x0404, 0x0303, 0x0300, 0x0407, 0x1c1f,
                       0x0206, 0x0102, 0x0404, 0x0303, 0x0300, 0x0407, 0x1c1f,

    /* GRAM and Base Imagen settings (ili9326 DS) */
    MREG16(2),  0x400, 0x001d, 0x0001,
    MREG16(1),  0x205, 0x0060,

    /* Power settings */
    MREG16(1),  0x007, 0x0001,
    MREG16(1),  0x031, 0x0071,
    MREG16(1),  0x110, 0x0001,
    MREG16(6),  0x100, 0x17b0, 0x0220, 0x00bd, 0x1500, 0x0103, 0x0105,

    /* Display On */
    MREG16(1),  0x007, 0x0021,
    MREG16(1),  0x001, 0x0110,
    MREG16(1),  0x003, 0x0230,
    MREG16(1),  0x002, 0x0500,
    MREG16(1),  0x007, 0x0031,
    MREG16(1),  0x030, 0x0007,
    SLEEP16(3),
    MREG16(1),  0x030, 0x03ff,
    SLEEP16(6),
    MREG16(1),  0x007, 0x0072,
    SLEEP16(15),
    MREG16(1),  0x007, 0x0173,
    END
};

#ifdef BOOTLOADER
static const uint8_t lcd_init_seq_0[] =
{
    CMD,   0x11,  0,        /* Sleep Out */
    SLEEP, 3,               /* 30 ms */
    CMD,   0x35,  1, 0x00,  /* Tearing Effect Line On */
    CMD,   0x3a,  1, 0x06,  /* Pixel Format Set */
    CMD,   0x36,  1, 0x00,  /* Memory Access Control */

    CMD,   0x13,  0,        /* Normal Mode On */
    CMD,   0x29,  0,        /* Display On */
    END
};

static const uint8_t lcd_init_seq_1[] =
{
    CMD,   0xb0, 21, 0x3a, 0x3a, 0x80, 0x80, 0x0a, 0x0a, 0x0a, 0x0a,
                     0x0a, 0x0a, 0x0a, 0x0a, 0x3c, 0x30, 0x0f, 0x00,
                     0x01, 0x54, 0x06, 0x66, 0x66,
    CMD,   0xb8,  1, 0xd8,
    CMD,   0xb1, 30, 0x14, 0x59, 0x00, 0x15, 0x57, 0x27, 0x04, 0x85,
                     0x14, 0x59, 0x00, 0x15, 0x57, 0x27, 0x04, 0x85,
                     0x14, 0x09, 0x15, 0x57, 0x27, 0x04, 0x05,
                     0x14, 0x09, 0x15, 0x57, 0x27, 0x04, 0x05,
    CMD,   0xd2,  1, 0x01,

    /* Gamma settings */
    CMD,   0xe0, 13, 0x00, 0x00, 0x00, 0x05, 0x0b, 0x12, 0x16, 0x1f,
                     0x25, 0x22, 0x24, 0x29, 0x1c,
    CMD,   0xe1, 13, 0x08, 0x01, 0x01, 0x06, 0x0b, 0x11, 0x15, 0x1f,
                     0x27, 0x26, 0x29, 0x2f, 0x1e,
    CMD,   0xe2, 13, 0x07, 0x01, 0x01, 0x05, 0x09, 0x0f, 0x13, 0x1e,
                     0x26, 0x25, 0x28, 0x2e, 0x1e,
    CMD,   0xe3, 13, 0x00, 0x00, 0x00, 0x05, 0x0b, 0x12, 0x16, 0x1f,
                     0x25, 0x22, 0x24, 0x29, 0x1c,
    CMD,   0xe4, 13, 0x08, 0x01, 0x01, 0x06, 0x0b, 0x11, 0x15, 0x1f,
                     0x27, 0x26, 0x29, 0x2f, 0x1e,
    CMD,   0xe5, 13, 0x07, 0x01, 0x01, 0x05, 0x09, 0x0f, 0x13, 0x1e,
                     0x26, 0x25, 0x28, 0x2e, 0x1e,

    CMD,   0x3a,  1, 0x06,  /* Pixel Format Set */
    CMD,   0xc2,  1, 0x00,  /* Power Control */
    CMD,   0x35,  1, 0x00,  /* Tearing Effect Line On */

    CMD,   0x11,  0,        /* Sleep Out */
    SLEEP, 6,               /* 60 ms */
    CMD,   0x13,  0,        /* Normal Mode On */
    CMD,   0x29,  0,        /* Display On */
    END
};
#endif /* BOOTLOADER */
#endif /* HAVE_LCD_SLEEP || BOOTLOADER */


static struct lcd_info_rec lcd_info_list[] =
{
    [0] = {
        .lcd_type   = 0,
        .mpuiface   = LCD_MPUIFACE_PAR18,
        .cmdset     = LCD_CMDSET_8BIT,
    #if defined(HAVE_LCD_SLEEP) || defined(HAVE_LCD_SHUTDOWN)
        .seq_sleep  = (void*) lcd_sleep_seq_01,
    #endif
    #ifdef HAVE_LCD_SLEEP
        .seq_awake  = (void*) lcd_awake_seq_01,
    #endif
    #ifdef BOOTLOADER
        .seq_init   = (void*) lcd_init_seq_0,
    #endif
    },

    [1] = {
        .lcd_type   = 1,
        .mpuiface   = LCD_MPUIFACE_PAR18,
        .cmdset     = LCD_CMDSET_8BIT,
    #if defined(HAVE_LCD_SLEEP) || defined(HAVE_LCD_SHUTDOWN)
        .seq_sleep  = (void*) lcd_sleep_seq_01,
    #endif
    #ifdef HAVE_LCD_SLEEP
        .seq_awake  = (void*) lcd_awake_seq_01,
    #endif
    #ifdef BOOTLOADER
        .seq_init   = (void*) lcd_init_seq_1,
    #endif
    },

    [2] = {
        .lcd_type   = 2,
        .mpuiface   = LCD_MPUIFACE_PAR18,
        .cmdset     = LCD_CMDSET_16BIT,
    #if defined(HAVE_LCD_SLEEP) || defined(HAVE_LCD_SHUTDOWN)
        .seq_sleep  = (void*) lcd_deepstandby_seq_23,
    #endif
    #ifdef HAVE_LCD_SLEEP
        .seq_awake  = (void*) lcd_init_seq_23,
    #endif
    #ifdef BOOTLOADER
        .seq_init   = (void*) lcd_init_seq_23,
    #endif
    },

    [3] = {
        .lcd_type   = 3,
        .mpuiface   = LCD_MPUIFACE_PAR18,
        .cmdset     = LCD_CMDSET_16BIT,
    #if defined(HAVE_LCD_SLEEP) || defined(HAVE_LCD_SHUTDOWN)
        .seq_sleep  = (void*) lcd_deepstandby_seq_23,
    #endif
    #ifdef HAVE_LCD_SLEEP
        .seq_awake  = (void*) lcd_init_seq_23,
    #endif
    #ifdef BOOTLOADER
        .seq_init   = (void*) lcd_init_seq_23,
    #endif
    },
};

struct lcd_info_rec* lcd_target_get_info(void)
{
    return &lcd_info_list[(PDAT6 & 0x30) >> 4];
}

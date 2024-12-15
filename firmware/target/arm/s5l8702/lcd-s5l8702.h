/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:
 *
 * Copyright © 2009 Michael Sparmann
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
#ifndef __LCD_S5L8702_H__
#define __LCD_S5L8702_H__

#include <stdint.h>

#include "config.h"
#include "lcd-target.h"


/* sequence actions */
enum {
    CMD = 0,    /* send command with N data */
    SLEEP,      /* unit: RB tick */                 // TODO: the unit in ms. but using HZ
#ifdef S5L_LCD_WITH_CMDSET16
    MREG,       /* write multiple registers */
    DELAY,      /* unit: 64 us. */
#endif
    END = 0xff
};

#ifdef S5L_LCD_WITH_CMDSET16
/* pack 16-bit sequence actions */
#define CMD16           (CMD)   /* send command with no data */             // TODO: why not use MREG16(0) ???
#define MREG16(len)     (MREG | ((len) << 8))
#define SLEEP16(t)      (SLEEP | ((t) << 8))
#define DELAY16(t)      (DELAY | ((t) << 8))

/* Supported command sets */
enum {
    LCD_CMDSET_8BIT = 0,    /* 8-bit command set similar to TODO */
    LCD_CMDSET_16BIT,       /* 16-bit command set similar to TODO */
};
#endif

/* Supported MPU interfaces */
enum {
    LCD_MPUIFACE_PAR9 = 0,  /* 8080-series 9-bit parallel */
    LCD_MPUIFACE_PAR18,     /* 8080-series 18-bit parallel */
    LCD_MPUIFACE_SERIAL,    /* 3-pin SPI (TBC) */
};


// TODO: typedef struct {} lcd_info_t;
struct lcd_info_rec {
    uint8_t lcd_type;
    uint8_t mpuiface;   /* LCD_MPUIFACE_ */
#ifdef S5L_LCD_WITH_CMDSET16
    uint8_t cmdset;     /* LCD_CMDSET_ */
#endif
    /* sequences */
#if defined(HAVE_LCD_SLEEP) || defined(HAVE_LCD_SHUTDOWN)
    void *seq_sleep;
#endif
#ifdef HAVE_LCD_SLEEP
    void *seq_awake;
#endif
#ifdef BOOTLOADER
    void *seq_init;
#endif
};

void lcd_awake(void);

#ifdef S5L_LCD_WITH_READID
void lcd_read_display_id(int mupiface, uint8_t *lcd_id);
#endif

extern struct lcd_info_rec* lcd_target_get_info(void);

#endif /* __LCD_S5L8702_H__ */

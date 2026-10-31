/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Aidan MacDonald
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
#ifndef __LCD_EROSQNATIVE_H__
#define __LCD_EROSQNATIVE_H__

#include <stdint.h>

extern uint32_t eros_qn_lcd_id;         /* 24-bit value from command 04h (read display ID) */
extern uint32_t eros_qn_lcd_id4;        /* 24-bit value from command D3h (read ID4) */

/* type of LCD */
extern uint32_t eros_qn_lcd_type;

/* whether lcd_type was matched from lcd_id or not */
extern int eros_qn_lcd_autodetected;

/* supported LCD types */
#define EROS_QN_LCDTYPE_UNKNOWN   0
#define EROS_QN_LCDTYPE_ILI9342C  1
#define EROS_QN_LCDTYPE_GC9308    2

#endif /* __LCD_EROSQNATIVE_H__ */

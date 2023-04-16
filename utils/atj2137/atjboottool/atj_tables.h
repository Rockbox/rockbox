/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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
#ifndef __ATJ_TABLES__
#define __ATJ_TABLES__

extern uint8_t g_decode_A_table[1024];
extern uint8_t g_decode_B_table[20];
extern uint32_t g_sect233k1_G_x[8];
extern uint32_t g_sect233k1_G_y[8];
extern uint32_t g_sect233k1_b[8];
extern uint32_t g_sect163r2_G_x[6];
extern uint32_t g_sect163r2_G_y[6];
extern uint32_t g_sect163r2_a[6];
extern uint32_t g_sect163r2_b[6];
extern uint32_t g_sect233k1_a[8];

#endif // __ATJ_TABLES__

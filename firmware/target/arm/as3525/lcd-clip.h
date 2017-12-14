/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008-2009 Rafaël Carré
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
#ifndef __LCDCLIP_H__
#define __LCDCLIP_H__

#include "config.h"

#if !defined(BOOTLOADER) && defined(SANSA_CLIPPLUS) /* only tested for clipplus */
/* Ensure empty FIFO for lcd commands is at least 3 deep */
#define LCD_USE_FIFO_FOR_COMMANDS
#endif

void lcd_write_cmd_triplet(int cmd1, int cmd2, int cmd3);

/* return variant number: 0 = clipv1, clipv2, old clip+, 1 = newer clip+ */
int lcd_hw_init(void) INIT_ATTR;

/* target-specific power enable */
void lcd_enable_power(bool onoff);

#endif /*__LCDCLIP_H__*/

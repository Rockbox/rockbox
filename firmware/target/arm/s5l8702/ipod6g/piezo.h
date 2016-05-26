/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Robert Keevil
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
#ifndef __PIEZO_H__
#define __PIEZO_H__

void piezo_init(void);
void piezo_stop(void);
void piezo_clear(void);
bool piezo_busy(void);
void piezo_button_beep(bool beep, bool force);

#ifdef BOOTLOADER
#include <inttypes.h>
void piezo_tone(uint32_t period, int32_t duration);
void piezo_seq(uint16_t *seq);
#endif

#endif /* __PIEZO_H__ */

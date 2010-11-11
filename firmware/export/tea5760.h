/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 * Tuner header for the Philips TEA5760
 *
 * Copyright (C) 2009 Bertrik Sikken
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

#ifndef _TEA5760_H_
#define _TEA5760_H_

#include "config.h"
#include "tuner.h"

#define HAVE_RADIO_REGION
#define HAVE_RADIO_RSSI

struct tea5760_dbg_info
{
    unsigned char read_regs[16];
    unsigned char write_regs[7];
};

int tea5760_set(int setting, int value);
int tea5760_get(int setting);
void tea5760_init(void);
void tea5760_dbg_info(struct tea5760_dbg_info *info);

#ifndef CONFIG_TUNER_MULTI
#define tuner_set tea5760_set
#define tuner_get tea5760_get
#endif

#endif /* _TEA5760_H_ */


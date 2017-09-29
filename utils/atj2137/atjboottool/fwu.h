/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Amaury Pouly
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
#ifndef __FWU_H__
#define __FWU_H__

#include <stdint.h>

enum fwu_mode_t
{
    FWU_AUTO, /* Will try to guess which mode to use */
    FWU_ATJ213X, /* Will use ATJ213x style mode */
    FWU_ATJ2127, /* Will use ATJ2127 variation */
};

/* Decrypt a FWU file inplace, the size variable is updated to reflect the size of the decrypted
 * firmware. Return 0 on success. The mode parameter selects how the function guesses between
 * various variants of FWU. */
int fwu_decrypt(uint8_t *buf, size_t *size, enum fwu_mode_t mode);
/* Check if a file looks like a FWU file */
bool fwu_check(uint8_t *buf, size_t size);

#endif /* __FWU_H__ */

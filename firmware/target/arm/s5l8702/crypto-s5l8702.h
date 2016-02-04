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
#ifndef __CRYPTO_S5L8702_H__
#define __CRYPTO_S5L8702_H__

#include <stdint.h>

#include "config.h"

#define SHA1_SZ     20  /* bytes */

enum hwkeyaes_direction
{
    HWKEYAES_DECRYPT = 0,
    HWKEYAES_ENCRYPT = 1
};

enum hwkeyaes_keyidx
{
    HWKEYAES_GKEY = 1,  /* device model key */
    HWKEYAES_UKEY = 2   /* device unique key */
};

void hwkeyaes(enum hwkeyaes_direction direction,
        uint32_t keyidx, void* data, uint32_t size);
void sha1(void* data, uint32_t size, void* hash);

#endif /* __CRYPTO_S5L8702_H__ */

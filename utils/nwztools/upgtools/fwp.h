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
#ifndef __fwp_h__
#define __fwp_h__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NWZ_KAS_SIZE    32
#define NWZ_KEYSIG_SIZE 16
#define NWZ_KEY_SIZE    8
#define NWZ_SIG_SIZE    8
#define NWZ_EXPKEY_SIZE (NWZ_KEY_SIZE * NWZ_KEY_SIZE)
#define NWZ_DES_BLOCK   8
#define NWZ_MD5_SIZE    16

/* size must be a multiple of 8 */
void fwp_read(void *in, int size, void *out, uint8_t *key);
void fwp_write(void *in, int size, void *out, uint8_t *key);
void fwp_setkey(char key[8]);
void fwp_crypt(void *buf, int size, int mode);

#ifdef __cplusplus
}
#endif

#endif /* __fwp_h__ */

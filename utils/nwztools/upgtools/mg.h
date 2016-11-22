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
#ifndef __mg_h__
#define __mg_h__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* size must be a multiple of 8 */
void mg_decrypt_fw(void *in, int size, void *out, uint8_t *key);
void mg_encrypt_fw(void *in, int size, void *out, uint8_t *key);
void mg_decrypt_pass(void *in, int size, void *out, uint8_t *key);
void mg_encrypt_pass(void *in, int size, void *out, uint8_t *key);
#ifdef __cplusplus
}
#endif

#endif /* __mg_h__ */

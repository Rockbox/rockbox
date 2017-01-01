/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Amaury Pouly
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
#include "crypto.h"
#include "misc.h"

static enum crypto_method_t g_cur_method = CRYPTO_NONE;
static byte g_key[16];

int crypto_setup(struct crypto_key_t *key)
{
    g_cur_method = key->method;
    switch(g_cur_method)
    {
        case CRYPTO_KEY:
            memcpy(g_key, key->u.key, 16);
            return CRYPTO_ERROR_SUCCESS;
        default:
            return CRYPTO_ERROR_BADSETUP;
    }
}

int crypto_apply(
    byte *in_data, /* Input data */
    byte *out_data, /* Output data (or NULL) */
    int nr_blocks, /* Number of blocks (one block=16 bytes) */
    byte iv[16], /* Key */
    byte (*out_cbc_mac)[16], /* CBC-MAC of the result (or NULL) */
    bool encrypt)
{
    if(g_cur_method == CRYPTO_KEY)
    {
        cbc_mac(in_data, out_data, nr_blocks, g_key, iv, out_cbc_mac, encrypt);
        return CRYPTO_ERROR_SUCCESS;
    }
    else
        return CRYPTO_ERROR_BADSETUP;
}

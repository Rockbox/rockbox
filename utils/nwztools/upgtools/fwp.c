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
#include <string.h>
#include <stdlib.h>
#include "fwp.h"
#include "misc.h"
#include "mg.h"

void fwp_read(void *in, int size, void *out, uint8_t *key)
{
    mg_decrypt_fw(in, size, out, key);
}

void fwp_write(void *in, int size, void *out, uint8_t *key)
{
    mg_encrypt_fw(in, size, out, key);
}

static uint8_t g_key[NWZ_KEY_SIZE];

void fwp_setkey(char key[NWZ_KEY_SIZE])
{
    memcpy(g_key, key, NWZ_KEY_SIZE);
}

void fwp_crypt(void *buf, int size, int mode)
{
    while(size >= NWZ_KEY_SIZE)
    {
        if(mode)
            mg_decrypt_pass(buf, NWZ_KEY_SIZE, buf, g_key);
        else
            mg_encrypt_pass(buf, NWZ_KEY_SIZE, buf, g_key);
        buf += NWZ_KEY_SIZE;
        size -= NWZ_KEY_SIZE;
    }
    if(size != 0)
        abort(); /* size is not a multiple of 8 */
}

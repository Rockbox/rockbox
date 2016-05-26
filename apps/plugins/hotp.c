/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Franklin Wei
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

/* simple HOTP plugin */

#include "plugin.h"

#include "lib/sha1.h"

int HOTP(unsigned char *secret, size_t sec_len, uint64_t ctr, int digits)
{
    ctr = htobe64(ctr);
    unsigned char hash[20];
    if(hmac_sha1(secret, sec_len, &ctr, 8, hash))
    {
        return -1;
    }

    int offs = hash[19] & 0xF;
    uint32_t code = (hash[offs] & 0x7F) << 24 |
        hash[offs + 1] << 16 |
        hash[offs + 2] << 8  |
        hash[offs + 3];

    int mod = 1;
    for(int i = 0; i < digits; ++i)
        mod *= 10;
    return code % mod;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    rb->splashf(HZ * 2, "%d", HOTP("12345678901234567890", strlen("12345678901234567890"), 1, 6));

    /* tell Rockbox that we have completed successfully */
    return PLUGIN_OK;
}

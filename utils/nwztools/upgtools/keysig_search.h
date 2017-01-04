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
#ifndef __keysig_search_h__
#define __keysig_search_h__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "fwp.h"

enum keysig_search_method_t
{
    KEYSIG_SEARCH_NONE = 0,
    KEYSIG_SEARCH_FIRST,
    KEYSIG_SEARCH_XDIGITS = KEYSIG_SEARCH_FIRST,
    KEYSIG_SEARCH_XDIGITS_UP,
    KEYSIG_SEARCH_ALNUM,
    KEYSIG_SEARCH_LAST
};

/* notify returns true if the key seems ok */
typedef bool (*keysig_notify_fn_t)(void *user, uint8_t key[NWZ_KEY_SIZE],
    uint8_t sig[NWZ_SIG_SIZE]);

struct keysig_search_desc_t
{
    const char *name;
    const char *comment;
};

struct keysig_search_desc_t keysig_search_desc[KEYSIG_SEARCH_LAST];

bool keysig_search(int method, uint8_t *enc_buf, size_t buf_sz,
    keysig_notify_fn_t notify, void *user, int nr_threads);

#endif /* __keysig_search_h__ */

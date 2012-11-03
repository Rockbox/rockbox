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

enum keysig_search_method_t
{
    KEYSIG_SEARCH_NONE = 0,
    KEYSIG_SEARCH_FIRST,
    KEYSIG_SEARCH_ASCII_STUPID = KEYSIG_SEARCH_FIRST,
    KEYSIG_SEARCH_ASCII_BRUTE,
    KEYSIG_SEARCH_LAST
};

/* notify returns true if the key seems ok */
typedef bool (*keysig_notify_fn_t)(void *user, uint8_t key[8], uint8_t sig[8]);
/* returns true if a key was accepted by notify */
typedef bool (*keysig_search_fn_t)(uint8_t *cipher, keysig_notify_fn_t notify, void *user);

struct keysig_search_desc_t
{
    const char *name;
    const char *comment;
    keysig_search_fn_t fn;
};

struct keysig_search_desc_t keysig_search_desc[KEYSIG_SEARCH_LAST];

#endif /* __keysig_search_h__ */

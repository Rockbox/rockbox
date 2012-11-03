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
#include "keysig_search.h"
#include "misc.h"
#include "mg.h"
#include <string.h>

#define HEX_MAJ

static uint8_t g_cipher[8];
static keysig_notify_fn_t g_notify;
static uint8_t g_key[8];
static void *g_user;
static bool is_hex[256];
static bool is_init = false;
#ifdef HEX_MAJ
static char hex_digits[] = "02468ABEF";
#else
static char hex_digits[] = "02468abef";
#endif

static void keysig_search_init()
{
    if(is_init) return;
    is_init = true;
    memset(is_hex, 0, sizeof(is_hex));
    for(int i = '0'; i <= '9'; i++)
        is_hex[i] = true;
#ifdef HEX_MAJ
    for(int i = 'A'; i <= 'F'; i++)
#else
    for(int i = 'a'; i <= 'f'; i++)
#endif
        is_hex[i] = true;
}

static inline bool is_full_ascii(uint8_t *arr)
{
    for(int i = 0; i < 8; i++)
        if(!is_hex[arr[i]])
            return false;
    return true;
}

static inline bool check_stupid()
{
    uint8_t res[8];
    mg_decrypt_fw(g_cipher, 8, res, g_key);
    if(is_full_ascii(res))
        return g_notify(g_user, g_key, res);
    return false;
}

static bool search_stupid_rec(int rem_digit, int rem_letter, int pos)
{
    if(pos == 8)
        return check_stupid();
    if(rem_digit > 0)
    {
        for(int i = '0'; i <= '9'; i += 2)
        {
            g_key[pos] = i;
            if(search_stupid_rec(rem_digit - 1, rem_letter, pos + 1))
                return true;
        }
    }
    if(rem_letter > 0)
    {
#ifdef HEX_MAJ
        for(int i = 'a' - 1; i <= 'f'; i += 2)
#else
        for(int i = 'A' - 1; i <= 'F'; i += 2)
#endif
        {
            g_key[pos] = i;
            if(search_stupid_rec(rem_digit, rem_letter - 1, pos + 1))
                return true;
        }
    }
    return false;
}

static bool search_stupid(int rem_digit, int rem_letter)
{
    return search_stupid_rec(rem_digit, rem_letter, 0);
}

bool keysig_search_ascii_stupid(uint8_t *cipher, keysig_notify_fn_t notify, void *user)
{
    keysig_search_init();
    memcpy(g_cipher, cipher, 8);
    g_notify = notify;
    g_user = user;
#if 1
    return search_stupid(4, 4) ||
        search_stupid(3, 5) || search_stupid(5, 3) ||
        search_stupid(2, 6) || search_stupid(6, 2) ||
        search_stupid(1, 7) || search_stupid(7, 1) ||
        search_stupid(0, 8) || search_stupid(8, 0);
#else
#define do(i) for(int a##i = 0; a##i < sizeof(hex_digits); a##i++) { g_key[i] = hex_digits[a##i];
#define od() }

    do(0)do(1)do(2)do(3)do(4)do(5)do(6)do(7)
        if(check_stupid()) return true;
    od()od()od()od()od()od()od()od()
#undef do
#undef od
    return false;
#endif
}

bool keysig_search_ascii_brute(uint8_t *cipher, keysig_notify_fn_t notify, void *user)
{
    keysig_search_init();
    return false;
}

struct keysig_search_desc_t keysig_search_desc[KEYSIG_SEARCH_LAST] =
{
    [KEYSIG_SEARCH_NONE] =
    {
        .name = "none",
        .fn = NULL,
        .comment = "don't use",
    },
    [KEYSIG_SEARCH_ASCII_STUPID] =
    {
        .name = "ascii-stupid",
        .fn = keysig_search_ascii_stupid,
        .comment = "Try to find a balance ascii key ignoring lsb"
    },
    [KEYSIG_SEARCH_ASCII_BRUTE] =
    {
        .name = "ascii-brute",
        .fn = keysig_search_ascii_brute,
        .comment = "Brute force all ASCII keys"
    },
};

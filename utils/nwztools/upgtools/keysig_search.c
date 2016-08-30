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
#include <stdio.h>

/* Key search methods
 *
 * This code tries to find the key and signature of a device using an upgrade
 * file. It more or less relies on brute force and makes the following assumptions.
 * It assumes the key and the signature are hexadecimal strings (it appears to be
 * true thus far). The code lists all possible keys and decrypts the first
 * 8 bytes of the file. If the decrypted signature happens to be an hex string,
 * the code reports the key and signature as potentially valid. Note that some
 * key/sig pairs may not be valid but since the likelyhood of decrypting a
 * random 8-byte sequence using an hex string key and to produce an hex string
 * is very small, there should be almost no false positive.
 *
 * Since the key is supposedly random, the code starts by looking at "balanced"
 * keys: keys with slightly more digits (0-9) than letters (a-f) and then moving
 * towards very unbalanced strings (only digits or only letters).
 */

static uint8_t g_cipher[8];
static keysig_notify_fn_t g_notify;
static uint8_t g_key[8];
static void *g_user;
static bool is_hex[256];
static bool is_init = false;
static uint64_t g_tot_count;
static uint64_t g_cur_count;
static int g_last_percent;
static int g_last_subpercent;

static void keysig_search_init()
{
    if(is_init) return;
    is_init = true;
    memset(is_hex, 0, sizeof(is_hex));
    for(int i = '0'; i <= '9'; i++)
        is_hex[i] = true;
    for(int i = 'a'; i <= 'f'; i++)
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
    // display progress
    g_cur_count++;
    int percent = (g_cur_count * 100ULL) / g_tot_count;
    int subpercent = ((g_cur_count * 1000ULL) / g_tot_count) % 10;
    if(percent != g_last_percent)
    {
        cprintf(RED, "%d%%", percent);
        fflush(stdout);
        g_last_subpercent = 0;
    }
    if(subpercent != g_last_subpercent)
    {
        cprintf(WHITE, ".");
        fflush(stdout);
    }
    g_last_percent = percent;
    g_last_subpercent = subpercent;

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
        for(int i = '0'; i <= '9'; i++)
        {
            g_key[pos] = i;
            if(search_stupid_rec(rem_digit - 1, rem_letter, pos + 1))
                return true;
        }
    }
    if(rem_letter > 0)
    {
        for(int i = 'a' - 1; i <= 'f'; i++)
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
    cprintf(WHITE, "\n    Looking for keys with ");
    cprintf(RED, "%d", rem_digit);
    cprintf(WHITE, " digits and ");
    cprintf(RED, "%d", rem_letter);
    cprintf(WHITE, " letters...");
    fflush(stdout);
    return search_stupid_rec(rem_digit, rem_letter, 0);
}

bool keysig_search_ascii_stupid(uint8_t *cipher, keysig_notify_fn_t notify, void *user)
{
    keysig_search_init();
    memcpy(g_cipher, cipher, 8);
    g_notify = notify;
    g_user = user;
    // compute number of possibilities
    g_cur_count = 0;
    g_tot_count = 1;
    g_last_percent = -1;
    for(int i = 0; i < 8; i++)
        g_tot_count *= 16ULL;
    cprintf(WHITE, "    Search space:");
    cprintf(RED, " %llu", (unsigned long long)g_tot_count);
    // sorted by probability:
    bool ret = search_stupid(5, 3) // 5 digits, 3 letters: 0.281632
        || search_stupid(6, 2) // 6 digits, 2 letters: 0.234693
        || search_stupid(4, 4) // 4 digits, 4 letters: 0.211224
        || search_stupid(7, 1) // 7 digits, 1 letters: 0.111759
        || search_stupid(3, 5) // 3 digits, 5 letters: 0.101388
        || search_stupid(2, 6) // 2 digits, 6 letters: 0.030416
        || search_stupid(8, 0) // 8 digits, 0 letters: 0.023283
        || search_stupid(1, 7) // 1 digits, 7 letters: 0.005214
        || search_stupid(0, 8);// 0 digits, 8 letters: 0.000391
    cprintf(OFF, "\n");
    return ret;
}

bool keysig_search_ascii_brute(uint8_t *cipher, keysig_notify_fn_t notify, void *user)
{
    (void) cipher;
    (void) notify;
    (void) user;
    keysig_search_init();
    cprintf(RED, "Unimplemented\n");
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
        .name = "ascii-hex",
        .fn = keysig_search_ascii_stupid,
        .comment = "Try to find an hexadecimal ascii string keysig"
    },
    [KEYSIG_SEARCH_ASCII_BRUTE] =
    {
        .name = "ascii-brute",
        .fn = keysig_search_ascii_brute,
        .comment = "Brute force all ASCII keys"
    },
};

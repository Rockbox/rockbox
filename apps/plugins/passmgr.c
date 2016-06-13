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

/* password manager plugin, supports both one-time static passwords */

/* see RFC 4226 and 6238 about the OTP algorithm */

#include "plugin.h"

#include "lib/aes.h"
#include "lib/display_text.h"
#include "lib/pluginlib_actions.h"
#include "lib/pluginlib_exit.h"
#include "lib/sha1.h"

#include "wordlist.h"

/* don't change these if you want to maintain backwards compatibility */
#define MAX_NAME    50
#define SECRET_MAX  256
#define URI_MAX     (MAX_NAME + SECRET_MAX * 2 + 1)
#define ACCT_FILE   PLUGIN_APPS_DATA_DIR "/passmgr.dat"

/* these can be changed without breaking anything */
#define PASS_MAX    128
#define KDF_MIN     5000 /* minimum KDF iterations */
#define KDF_MAX     2500000
#define KDF_DEFAULT (HZ / 4) /* decryption will take about this long by default */

/* convenience macros */
#define MAX(a, b) (((a)>(b))?(a):(b))
#define assert(x) (!(x)?assert_fail():0)

/* we define these no matter what */
#ifdef PASSMGR_DICEWARE
#define DICEWARE_WORDS 6
#else
#define DICEWARE_WORDS 0
#endif
#define DICEWARE_BUF (16 * DICEWARE_WORDS + 1)

struct account_t {
    char name[MAX_NAME];

    /* this numbering maintans some backwards compatibility: older
     * versions had a bool that would be false (zero) for HOTP
     * accounts, but gcc would pad it to 4 bytes; by using this
     * numbering very little additional logic is needed */
    enum { TYPE_HOTP = 0, TYPE_TOTP = 1, TYPE_STATIC = 3} type;

    union {
        uint64_t hotp_counter;
        int totp_period;
    };

    int digits;

    unsigned char secret[SECRET_MAX];
    int sec_len;
};

/* in plugin buffer */
static struct account_t *accounts = NULL;

/* global variables */

static int max_accts = 0; // dynamic, depends on plugin buffer size
static int next_slot = 0;

static int time_offs = 0; // in seconds
static int kdf_iters = 0; // calculated on first run
static char encrypted = 0; // 0 = off, 1 = password, 2 = diceware

static char enc_password[PASS_MAX + 1]; // encryption password
static char data_buf[MAX(MAX_NAME, MAX(SECRET_MAX * 2, MAX(20, MAX(URI_MAX, MAX(sizeof(struct account_t), DICEWARE_BUF)))))];
static char temp_sec[SECRET_MAX];
static long background_stack[2 * DEFAULT_STACK_SIZE / sizeof(long)];

static void wipe_buf(void *ptr, size_t len)
{
    rb->memset(ptr, 0, len);
    rb->memset(ptr, 0xff, len);
    rb->memset(ptr, 0, len);
}

static void erase_sensitive_info(void)
{
    wipe_buf(accounts, sizeof(struct account_t) * max_accts);
    wipe_buf(enc_password, sizeof(enc_password));
    wipe_buf(temp_sec, sizeof(temp_sec));
    wipe_buf(background_stack, sizeof(background_stack));
}

static void acct_menu(char *title, void (*cb)(int acct));

static void assert_fail(void)
{
    rb->splashf(HZ * 2, "Assertion failed! REPORT ME!");
    exit(0);
}

static int HOTP(unsigned char *secret, size_t sec_len, uint64_t ctr, int digits)
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

    // debug
    // rb->splashf(HZ * 5, "HOTP %*s, %llu, %d: %d", sec_len, secret, htobe64(ctr), digits, code % mod);

    return code % mod;
}

static bool compare_constant_time(volatile const char* p1, volatile const char* p2, size_t n)
{
    volatile char c = 0;
    for (size_t i=0; i<n; ++i)
        c |= p1[i] ^ p2[i];
    return (c == 0);
}

#if CONFIG_RTC
static time_t get_utc(void)
{
    return rb->mktime(rb->get_time()) - time_offs;
}

static int TOTP(unsigned char *secret, size_t sec_len, uint64_t step, int digits)
{
    if(!step)
        return -1;
    uint64_t tm = get_utc() / step;
    return HOTP(secret, sec_len, tm, digits);
}
#endif

/* search the accounts for a duplicate */
static bool acct_exists(const char *name)
{
    for(int i = 0; i < next_slot; ++i)
        if(!rb->strcmp(accounts[i].name, name))
            return true;
    return false;
}

/* diceware-related code */

#ifdef PASSMGR_DICEWARE
/* returns the index of the first element with a matching prefix, assumes sorted list */
static int search_prefix(const char **list, size_t list_len, const char *prefix, int *last)
{
    int l = 0;
    int r = list_len - 1;
    int len = rb->strlen(prefix);
    while(l <= r)
    {
        int m = (l + r) / 2;
        int c = rb->strncmp(list[m], prefix, len);
        if(c < 0)
            l = m + 1;
        else if(c > 0)
            r = m - 1;
        else
        {
            /* we can afford to be a bit slower here with a
             * linear-time algorithm */

            /* search forwards to find the last word with this
             * prefix */

            if(last)
            {
                int old_m = m;
                while((unsigned)m < list_len - 1)
                {
                    if(!rb->strncmp(list[m + 1], prefix, len))
                        m++;
                    else
                        break;
                }
                *last = m;
                m = old_m;
            }

            /* search backwards */
            while(m > 0)
            {
                if(!rb->strncmp(list[m - 1], prefix, len))
                    m--;
                else
                    break;
            }

            return m;
        }
    }
    return -1;
}

static char charlist_items[26][8];

static const char* charlist_cb(int selected_item, void *data,
                               char *buffer, size_t buffer_len)
{
    (void) data;
    char *str = charlist_items[selected_item];
    str[0] = toupper(str[0]);
    rb->snprintf(buffer, buffer_len, "%s-", str);
    str[0] = tolower(str[0]);
    return buffer;
}

static const char* wordlist_cb(int selected_item, void *data,
                               char *buffer, size_t buffer_len)
{
    rb->snprintf(buffer, buffer_len, "%s", word_list[(int)data + selected_item]);
    return buffer;
}

static char choose_letter(char *prefix, char selected, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char str[32];
    rb->vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);
    struct gui_synclist list;

    rb->gui_synclist_init(&list, &charlist_cb, NULL, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&list, NULL);
    rb->gui_synclist_select_item(&list, 0);

    int n_items = 0;
    for(char c = 'a'; c <= 'z'; ++c)
    {
        char temp[8];
        rb->snprintf(temp, sizeof(temp), "%s%c", prefix, c);
        if(search_prefix(word_list, word_list_len, temp, NULL) >= 0)
        {
            rb->strlcpy(charlist_items[n_items], temp, sizeof(charlist_items[n_items]));
            if(selected == c)
                rb->gui_synclist_select_item(&list, n_items);
            ++n_items;
        }
    }
    rb->gui_synclist_set_nb_items(&list, n_items);
    rb->gui_synclist_limit_scroll(&list, false);

    bool done = false;
    rb->gui_synclist_set_title(&list, str, NOICON);
    while (!done)
    {
        rb->gui_synclist_draw(&list);
        int button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        if(rb->gui_synclist_do_button(&list, &button, LIST_WRAP_ON))
            continue;

        switch (button)
        {
        case ACTION_STD_OK:
        {
            char *str = charlist_items[rb->gui_synclist_get_sel_pos(&list)];
            int len = rb->strlen(str);
            return str[len - 1];
        }
        case ACTION_STD_PREV:
        case ACTION_STD_CANCEL:
        case ACTION_STD_MENU:
            return '\0';
        }
        rb->yield();
    }
    return '\0';
}

/* prompt the user for a word from a list */
/* returns 0 on success, negative on failure */
static int choose_word(char *ret, size_t ret_len, const char **wordlist, size_t wordlist_len, int wordnr)
{
    char prefix[3], ch;

    char first_sel = '\0', second_sel = '\0';

    rb->memset(prefix, 0, sizeof(prefix));

first_letter:

    first_sel = ch = choose_letter(prefix, first_sel, "Choose First Letter of Word #%d", wordnr);
    if(!ch)
        return -1;
    prefix[0] = ch;

    if(search_prefix(wordlist, wordlist_len, prefix, NULL) < 0)
    {
        rb->splash(HZ, "No words with this prefix!");
        prefix[0] = '\0';
        goto first_letter;
    }

second_letter:

    second_sel = ch = choose_letter(prefix, second_sel, "Choose Second Letter of Word #%d", wordnr);
    if(!ch)
    {
        prefix[0] = '\0';
        goto first_letter;
    }
    prefix[1] = ch;

    if(search_prefix(wordlist, wordlist_len, prefix, NULL) < 0)
    {
        rb->splash(HZ, "No words with this prefix!");
        prefix[1] = '\0';
        goto second_letter;
    }

    int start, stop;
    start = search_prefix(wordlist, wordlist_len, prefix, &stop);

    /* now list the words with this prefix */

    struct gui_synclist list;

    rb->gui_synclist_init(&list, &wordlist_cb, (void*)start, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&list, NULL);
    rb->gui_synclist_set_nb_items(&list, stop - start + 1);
    rb->gui_synclist_limit_scroll(&list, false);
    rb->gui_synclist_select_item(&list, 0);

    bool done = false;
    int wordidx = -1;

    char str[32];
    rb->snprintf(str, sizeof(str), "Choose Word #%d", wordnr);
    rb->gui_synclist_set_title(&list, str, NOICON);
    while (!done)
    {
        rb->gui_synclist_draw(&list);
        int button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&list, &button, LIST_WRAP_ON))
            continue;

        switch (button)
        {
        case ACTION_STD_OK:
            wordidx = start + rb->gui_synclist_get_sel_pos(&list);
            done = true;
            break;
        case ACTION_STD_PREV:
        case ACTION_STD_CANCEL:
        case ACTION_STD_MENU:
            prefix[1] = '\0';
            goto second_letter;
        }
        rb->yield();
    }

    rb->strlcpy(ret, word_list[wordidx], ret_len);
    return 0;
}

static int read_diceware_passphrase(char *buf, size_t buflen)
{
    /* assumes that no words are > 15 chars */
    char words[DICEWARE_WORDS][16];

    for(int i = 0; i < DICEWARE_WORDS; ++i)
    {
        int rc = choose_word(words[i], sizeof(words[i]), word_list, word_list_len, i + 1);
        if(rc < 0)
        {
            if(!i)
                return -1; /* failure */
            else
                i -= 2; /* back a word */
        }
    }

    /* copy the words into the passphrase */
    buf[0] = '\0';
    for(int i = 0; i < DICEWARE_WORDS; ++i)
    {
        rb->strlcat(buf, words[i], buflen);
        rb->strlcat(buf, " ", buflen);
    }
    return 0;
}

#endif /* #ifdef PASSMGR_DICEWARE */

// Base32 implementation
//
// Copyright 2010 Google Inc.
// Author: Markus Gutschke
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

static int base32_decode(uint8_t *result, int bufSize, const uint8_t *encoded) {
    int buffer = 0;
    int bitsLeft = 0;
    int count = 0;
    for (const uint8_t *ptr = encoded; count < bufSize && *ptr; ++ptr) {
        uint8_t ch = *ptr;
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '-') {
            continue;
        }
        buffer <<= 5;

        // Deal with commonly mistyped characters
        if (ch == '0') {
            ch = 'O';
        } else if (ch == '1') {
            ch = 'L';
        } else if (ch == '8') {
            ch = 'B';
        }

        // Look up one base32 digit
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
            ch = (ch & 0x1F) - 1;
        } else if (ch >= '2' && ch <= '7') {
            ch -= '2' - 26;
        } else {
            return -1;
        }

        buffer |= ch;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            result[count++] = buffer >> (bitsLeft - 8);
            bitsLeft -= 8;
        }
    }
    if (count < bufSize) {
        result[count] = '\000';
    }
    return count;
}

static int base32_encode(const uint8_t *data, int length, uint8_t *result,
                         int bufSize) {
    if (length < 0 || length > (1 << 28)) {
        return -1;
    }
    int count = 0;
    if (length > 0) {
        int buffer = data[0];
        int next = 1;
        int bitsLeft = 8;
        while (count < bufSize && (bitsLeft > 0 || next < length)) {
            if (bitsLeft < 5) {
                if (next < length) {
                    buffer <<= 8;
                    buffer |= data[next++] & 0xFF;
                    bitsLeft += 8;
                } else {
                    int pad = 5 - bitsLeft;
                    buffer <<= pad;
                    bitsLeft += pad;
                }
            }
            int index = 0x1F & (buffer >> (bitsLeft - 5));
            bitsLeft -= 5;
            result[count++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"[index];
        }
    }
    if (count < bufSize) {
        result[count] = '\000';
    }
    return count;
}

/***********************************************************************
 * File browser (from rockpaint)
 ***********************************************************************/

static bool browse( char *dst, int dst_size, const char *start )
{
    struct browse_context browse;

    rb->browse_context_init(&browse, SHOW_ALL,
                            BROWSE_SELECTONLY|BROWSE_NO_CONTEXT_MENU,
                            NULL, NOICON, start, NULL);

    browse.buf = dst;
    browse.bufsize = dst_size;

    rb->rockbox_browse(&browse);

    return (browse.flags & BROWSE_SELECTED);
}

#if 0
/* check an entered password/diceware passphrase with the actual
 * one */
/* used as a very weak security measure */
static bool verify_password(void)
{
    char temp_pass[PASS_MAX];

    switch(encrypted)
    {
    case 0:
        return true;
    case 1:
        rb->splash(HZ, "Enter current password:");
        temp_pass[0] = '\0';
        if(rb->kbd_input(temp_pass, sizeof(temp_pass)) < 0)
            return false;
        break;
#ifdef PASSMGR_DICEWARE
    case 2:
        rb->splash(HZ, "Enter current passphrase:");
        if(read_diceware_passphrase(temp_pass, sizeof(temp_pass)) < 0)
            return false;
        break;
#endif
    }
    if(rb->strcmp(enc_password, temp_pass))
    {
        rb->splashf(HZ * 2, "Wrong password!");
        return false;
    }
    return true;
}
#endif

/* a simple AES128-CTR implementation */

struct aes_ctr_ctx {
    char key[16];
    union {
        char bytes[16];
        uint64_t half[2];
    } counter;
    /* one block */
    char keystream[16];
    uint8_t bytes_left;
};

static void aes_ctr_init(struct aes_ctr_ctx *ctx, const char *key, uint64_t nonce)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    rb->memcpy(ctx->key, key, 16);
    ctx->counter.half[0] = nonce;
    ctx->counter.half[1] = 0;
    ctx->bytes_left = 0;
}

static void aes_ctr_nextblock(struct aes_ctr_ctx *ctx)
{
    AES128_ECB_encrypt((char*)&ctx->counter, ctx->key, ctx->keystream);
    ctx->counter.half[1]++;
    ctx->bytes_left = 16;
}

/* should be safe to operate in-place */
static void aes_ctr_process(struct aes_ctr_ctx *ctx, const unsigned char *in, unsigned char *out, size_t len)
{
    while(len--)
    {
        if(!ctx->bytes_left)
            aes_ctr_nextblock(ctx);
        *out++ = *in++ ^ ctx->keystream[16 - ctx->bytes_left--];
    }
}

static void aes_ctr_destroy(struct aes_ctr_ctx *ctx)
{
    wipe_buf(ctx, sizeof(*ctx));
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

/* yield after this many HMAC iterations */
#define YIELD_INTERVAL 2500

/* internal PBKDF function */
#if CONFIG_CPU == S5L8702 && !defined(SIMULATOR)

/* hardware-accelerated version */

static void PBKDF2_F(const void *pass, size_t passlen, const void *salt, size_t saltlen,
                     int c, uint32_t blockidx, void *tmp, char *out)
{
    char buf[64 + 20];
    char *last = buf + 64;

    rb->yield();

    rb->memcpy(tmp, salt, saltlen);
    blockidx = htobe32(blockidx);
    rb->memcpy(tmp + saltlen, &blockidx, 4);

    hmac_sha1(pass, passlen, tmp, saltlen + 4, last);
    rb->memcpy(out, last, 20);

    /* begin micro-optimization :P */
    for(int j = 0; j < c / YIELD_INTERVAL; ++j)
    {
        int iters = YIELD_INTERVAL;
        if(c % YIELD_INTERVAL == 0 && j == c / YIELD_INTERVAL - 1)
            iters--;
        for(int i = 0; i < iters; ++i)
        {
            hmac_sha1_hwaccel(pass, passlen, last, 20, last);

            uint32_t *a = (uint32_t*)out;
            const uint32_t *b = (const uint32_t*)last;

            /* out ^= last: */
            *a++ ^= *b++;
            *a++ ^= *b++;
            *a++ ^= *b++;
            *a++ ^= *b++;
            *a++ ^= *b++;
        }
        rb->yield();
    }
    for(int i = 1; i < c % YIELD_INTERVAL; ++i)
    {
        hmac_sha1_hwaccel(pass, passlen, last, 20, last);

        uint32_t *a = (uint32_t*)out;
        const uint32_t *b = (const uint32_t*)last;

        /* out ^= last: */
        *a++ ^= *b++;
        *a++ ^= *b++;
        *a++ ^= *b++;
        *a++ ^= *b++;
        *a++ ^= *b++;
    }
    rb->yield();
}
#else

/* all-software version */

static void PBKDF2_F(const void *pass, size_t passlen, const void *salt, size_t saltlen,
                     int c, uint32_t blockidx, void *tmp, char *out)
{
    char last[20];

    rb->yield();

    rb->memcpy(tmp, salt, saltlen);
    blockidx = htobe32(blockidx);
    rb->memcpy(tmp + saltlen, &blockidx, 4);

    hmac_sha1(pass, passlen, tmp, saltlen + 4, last);
    rb->memcpy(out, last, 20);

    /* begin micro-optimization :P */
    for(int j = 0; j < c / YIELD_INTERVAL; ++j)
    {
        int iters = YIELD_INTERVAL;
        if(c % YIELD_INTERVAL == 0 && j == c / YIELD_INTERVAL - 1)
            iters--;
        for(int i = 0; i < iters; ++i)
        {
            hmac_sha1(pass, passlen, last, 20, last);

            uint32_t *a = (uint32_t*)out;
            const uint32_t *b = (const uint32_t*)last;

            /* out ^= last: */
            *a++ ^= *b++;
            *a++ ^= *b++;
            *a++ ^= *b++;
            *a++ ^= *b++;
            *a++ ^= *b++;
        }
        rb->yield();
    }
    for(int i = 1; i < c % YIELD_INTERVAL; ++i)
    {
        hmac_sha1(pass, passlen, last, 20, last);

        uint32_t *a = (uint32_t*)out;
        const uint32_t *b = (const uint32_t*)last;

        /* out ^= last: */
        *a++ ^= *b++;
        *a++ ^= *b++;
        *a++ ^= *b++;
        *a++ ^= *b++;
        *a++ ^= *b++;
    }
    rb->yield();
}
#endif

/* uses HMAC-SHA-1 as the underlying PRF */
/* tmp must be at least saltlen + 4 bytes */
static void PBKDF2(const void *pass, size_t passlen, const void *salt, size_t saltlen,
                   int c, char *dk, size_t dklen, void *tmp)
{
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    /* number of blocks */
    unsigned l = dklen / 20;
    if(dklen % 20)
        l += 1; // round up

    /* amount of left-over bytes in the final block */
    unsigned r = dklen - (l - 1) * 20;

    for(uint32_t i = 1; i < l; ++i)
    {
        PBKDF2_F(pass, passlen, salt, saltlen, c, i, tmp, dk);
        dk += 20;
    }
    if(r)
    {
        char temp_block[20];
        PBKDF2_F(pass, passlen, salt, saltlen, c, l, tmp, temp_block);
        rb->memcpy(dk, temp_block, r);
    }
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
}

/* calculate about how many KDF iterations it takes to make key
 * derivation take a certain time */
static int calc_kdf_iters(long delay)
{
    rb->splash(0, "Please wait...");
    int iters = KDF_MIN;
    long ticks = 0;
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    /* calculate how many iterations make PBKDF2 take a certain time */
    while(ticks < 4 && iters <= KDF_MAX)
    {
        char out[20];
        char tmp[4 + 4];
        long start = *rb->current_tick;
        PBKDF2("password", 8, "salt", 4, KDF_MIN, out, 20, tmp);
        long end = *rb->current_tick;
        ticks = end - start;
        if(!ticks)
            iters *= 2;
    }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    if(!ticks)
        return KDF_MAX;

    /* then extrapolate to the desired time */
    int ret = (delay * iters) / ticks;
    rb->lcd_update();
    return ret < KDF_MIN ? KDF_MIN : ret;
}

static int read_password_or_passphrase(char *buf, size_t len)
{
    buf[0] = '\0';
    switch(encrypted)
    {
    case 1: /* plain password */
        rb->splash(HZ, "Enter password:");
        return rb->kbd_input(buf, len);
    case 2: /* passphrase */
#ifdef PASSMGR_DICEWARE
        rb->splash(HZ, "Enter passphrase:");
        return read_diceware_passphrase(buf, len);
#endif
    default:
        return -1;
    }
}

static bool read_accts(void)
{
    int fd = rb->open(ACCT_FILE, O_RDONLY);
    if(fd < 0)
        return false;

    unsigned char buf[4];
    /* two versions to maintain backwards-compatibility */
    const char *magic_old = "OTP1";
    const char *magic = "OTP2";
    rb->read(fd, buf, 4);
    if(rb->memcmp(magic, buf, 4) && rb->memcmp(magic_old, buf, 4))
    {
        rb->splash(HZ * 2, "Corrupt save data!");
        rb->close(fd);
        return false;
    }

    rb->read(fd, &time_offs, sizeof(time_offs));

    if(!rb->memcmp(magic, buf, 4))
    {
        /* version 2 */
        rb->read(fd, &encrypted, sizeof(encrypted));
        rb->read(fd, &kdf_iters, sizeof(kdf_iters));

        if(encrypted)
        {
            uint64_t nonce;
            rb->read(fd, &nonce, sizeof(nonce));

            /* read in the MAC */
            char mac_given[20];
            rb->read(fd, mac_given, 20);

            /* also read the encrypted data into memory */
            while(next_slot < max_accts)
            {
                if(rb->read(fd, accounts + next_slot, sizeof(struct account_t)) != sizeof(struct account_t))
                    break;
                ++next_slot;
            }

            rb->close(fd);

            for(int i = 0; i < 3; ++i)
            {
                if(read_password_or_passphrase(enc_password, sizeof(enc_password)) < 0)
                {
                    rb->close(fd);
                    exit(PLUGIN_ERROR);
                }

                rb->splash(0, "Decrypting...");

                /* derive the key */
                char key[20];
                char tmp[sizeof(nonce) + 4];

                //long start = *rb->current_tick;
                PBKDF2(enc_password, rb->strlen(enc_password), &nonce, sizeof(nonce),
                       kdf_iters, key, sizeof(key), tmp);
                //long end = *rb->current_tick;
                //rb->splashf(HZ, "Key derviation takes %ld ticks", end - start);

#if CONFIG_CPU == S5L8702 && !defined(SIMULATOR)
                /* if we have a hardware AES coprocessor with
                 * device-unique keys, use it to encrypt the key to
                 * tie it to this device */
                rb->s5l8702_hwkeyaes(HWKEYAES_ENCRYPT,
                                     HWKEYAES_UKEY,
                                     key, sizeof(key));
#endif

                /* calculate the MAC of the ciphertext to see if the
                 * password is correct before decrypting note that we
                 * only use 4 bytes of the derived key in calculating
                 * the MAC, this makes an attack more difficult and
                 * prone to false positives, which is good */
                char mac_calculated[20];
                hmac_sha1(key + 16, sizeof(key) - 16, accounts,
                          next_slot * sizeof(struct account_t), mac_calculated);

                if(!compare_constant_time(mac_calculated, mac_given, 20))
                {
                    rb->splash(HZ, "Wrong password!");
                    continue;
                }

                /* decrypt the data with AES128-CTR */
                struct aes_ctr_ctx aes_ctx;

                aes_ctr_init(&aes_ctx, key, nonce);

                aes_ctr_process(&aes_ctx, (const unsigned char*)accounts, (char*)accounts, sizeof(struct account_t) * next_slot);

                aes_ctr_destroy(&aes_ctx);

                /* successful decryption */
                return true;
            }

            exit(PLUGIN_ERROR);
        }
    }

    /* plain, unencrypted format */

    while(next_slot < max_accts)
    {
        if(rb->read(fd, accounts + next_slot, sizeof(struct account_t)) != sizeof(struct account_t))
            break;
        ++next_slot;
    }

    rb->close(fd);
    return true;
}

static struct mutex save_mutex;
static volatile bool quiet_save SHAREDDATA_ATTR = false;

static void save_accts(void)
{
    if(!quiet_save)
        rb->splash(0, "Saving...");
    rb->mutex_lock(&save_mutex);
    int fd = rb->open(ACCT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    rb->fdprintf(fd, "OTP2");

    rb->write(fd, &time_offs, sizeof(time_offs));
    rb->write(fd, &encrypted, sizeof(encrypted));

    /* write how many KDF iterations we use even if encryption is disabled */
    rb->write(fd, &kdf_iters, sizeof(kdf_iters));

    assert(sizeof(data_buf) >= sizeof(struct account_t));
    assert(sizeof(data_buf) >= 20); // needs to hold an SHA-1 hash

    if(encrypted)
    {
        /* encrypt the data with AES128-CTR */

        /* generate/write the nonce */
        uint64_t nonce = *rb->current_tick;
#if CONFIG_RTC
        nonce |= (uint64_t)get_utc() << 32;
#endif

        rb->write(fd, &nonce, sizeof(nonce));

        /* placeholder for the MAC */
        off_t mac_offs = rb->lseek(fd, 0, SEEK_CUR);
        rb->memset(data_buf, 0, 20);
        rb->write(fd, data_buf, 20);

        /* use PKCS #5 PBKDF2 to derive a strong key from the password */
        char key[20];
        char tmp[sizeof(nonce) + 4];

        PBKDF2(enc_password, rb->strlen(enc_password), &nonce, sizeof(nonce),
               kdf_iters, key, sizeof(key), tmp);

#if CONFIG_CPU == S5L8702 && !defined(SIMULATOR)
        /* if we have a hardware AES coprocessor with device-unique
         * keys, use it to encrypt the key to tie it to this device */
        rb->s5l8702_hwkeyaes(HWKEYAES_ENCRYPT,
                             HWKEYAES_UKEY,
                             key, sizeof(key));
#endif

        struct aes_ctr_ctx aes_ctx;
        aes_ctr_init(&aes_ctx, key, nonce);

        struct hmac_ctx hmac_ctx;
        hmac_sha1_init(&hmac_ctx, key + 16, sizeof(key) - 16);

        for(int i = 0; i < next_slot; ++i)
        {
            /* encrypt */
            aes_ctr_process(&aes_ctx, (unsigned char*)(accounts + i), data_buf, sizeof(struct account_t));

            rb->write(fd, data_buf, sizeof(struct account_t));

            /* then MAC */
            hmac_sha1_process_bytes(&hmac_ctx, data_buf, sizeof(struct account_t));
            rb->yield();
        }

        char mac[20];

        hmac_sha1_finish_ctx(&hmac_ctx, mac);

        rb->lseek(fd, mac_offs, SEEK_SET);
        rb->write(fd, mac, 20);

        aes_ctr_destroy(&aes_ctx);
    }
    else
        for(int i = 0; i < next_slot; ++i)
            rb->write(fd, accounts + i, sizeof(struct account_t));

    rb->close(fd);
    rb->mutex_unlock(&save_mutex);
    quiet_save = false;
}

/* generate a desired number of random bits */
/* note that len must be a multiple of sizeof(long) and >= 20 */
static void gather_entropy(void *out, size_t len)
{
    assert(len % sizeof(long) == 0);
    assert(len >= 20);
    rb->splash(0, "Gathering entropy... Please press the keys as randomly as possible.");
    /* fill the buffer with a seed */
    char buf[20];
    long *ptr = (long*)buf;

    /* mix in certain filesystem information such as the number of
     * files in the root directory */
    DIR *root = rb->opendir("/");
    int count = 0;
    while(rb->readdir(root))
        count++;

    for(unsigned i = 0; i < len / sizeof(long); ++i)
    {
        *ptr++ = *rb->current_tick ^ count;
        rb->yield();
    }

    ptr = (long*)buf;

    /* now we mix in the system settings and status */
    hmac_sha1(rb->global_settings, sizeof(struct user_settings), ptr, 20, ptr);
    hmac_sha1(rb->global_status, sizeof(struct system_status), ptr, 20, ptr);

    /* mix the keypresses into the pool */
    /* for now just gather a certain number of keypresses and their
     * associated timestamps */
    for(int keys = 0; keys < 80; ++keys)
    {
        uint32_t data = rb->button_get(true) ^ *rb->current_tick;
#ifdef HAVE_WHEEL_POSITION
        if(rb->wheel_status() >= 0)
            data ^= rb->wheel_status();
#endif

        /* XOR in environmental conditions */
        data ^= rb->audio_status() << 8;
        data ^= rb->audio_get_file_pos() << 16;
        data ^= rb->battery_voltage() << 24;

#ifdef USEC_TIMER
        /* we swap because we need more entropy in the high-order bits */
        data ^= SWAP32_CONST(USEC_TIMER);
#endif

        rb->splashf(0, "Data is 0x%lx", data);

        ptr = (long*)buf;

        /* XOR the data in */
        for(unsigned i = 0; i < len / sizeof(long); ++i)
            *ptr++ ^= data;

        long timestamp = *rb->current_tick;

#if CONFIG_RTC
        timestamp ^= get_utc();
#endif

        /* finally mix it all up with an HMAC */
        hmac_sha1(&timestamp, sizeof(long), ptr, 20, ptr);
        rb->yield();
    }

    /* as a final step we use PBKDF2 to stretch the 20 bytes we have
     * to the desired length */
    long salt = *rb->current_tick;
    char tmp[sizeof(salt) + 4];
    PBKDF2(buf, 20, &salt, sizeof(salt), 1, out, len, tmp);

    rb->button_clear_queue();
}

static void generate_random_password(char *dest, size_t len)
{
    rb->memset(dest, 0, len);
    const char *charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-=!@#$%^&*()_+[]{}\\|,.<>/?`~'\"";
    char seed[20];
    gather_entropy(seed, sizeof(seed));

    unsigned dest_idx = 0;

    /* now, iterate over bytes of the seed and append characters that
     * are in the character set */
    while(dest_idx < len - 1)
    {
        for(unsigned i = 0; i < sizeof(seed) && dest_idx < len - 1; ++i)
        {
            if(rb->strchr(charset, seed[i]) && seed[i])
                dest[dest_idx++] = seed[i];
        }
        sha1_buffer(seed, sizeof(seed), seed);
    }
}

/* 0: manual entry, 1: random pass, -1: fail */
static int static_password_menu(char *passbuf, size_t buflen)
{
    MENUITEM_STRINGLIST(static_type, "Static Password Options", NULL,
                        "Enter Manually",
                        "Generate Randomly",
                        "Cancel");

    while(1)
    {
        switch(rb->do_menu(&static_type, NULL, NULL, false))
        {
        case 0:
            rb->splash(HZ * 2, "Enter account password:");
            return 0;
        case 1:
        {
            rb->splash(HZ * 2, "Enter desired password length:");

            char temp_buf[16];
            temp_buf[0] = '\0';

            if(rb->kbd_input(temp_buf, sizeof(temp_buf)) < 0)
                return -1;

            int len = rb->atoi(temp_buf);
            if(len < 4 || len > (int)buflen)
            {
                rb->splash(HZ, "Password length not in allowed range!");
                return -1;
            }

            generate_random_password(passbuf, len + 1);
            return 1;
        }
        case 2:
        default:
            return -1;
        }
    }
}

static volatile bool kill_background SHAREDDATA_ATTR = false;
static volatile bool want_save SHAREDDATA_ATTR = false;
static int background_id = -1;

static void background_save(void)
{
    want_save = true;
    /* fall back to normal save */
    if(background_id < 0)
        save_accts();
}

static void background_thread(void)
{
    while(1)
    {
        if(want_save)
        {
            quiet_save = true;
            save_accts();
            want_save = false;
        }
        if(kill_background)
            rb->thread_exit();
        rb->sleep(HZ / 25);
    }
}

static int compare_acct(const void *a, const void *b)
{
    const struct account_t *a1 = a, *b1 = b;
    return rb->strcmp(a1->name, b1->name);
}

static void sort_accts(void)
{
    /* don't sort while a save is going on */
    rb->mutex_lock(&save_mutex);
    rb->qsort(accounts, next_slot, sizeof(struct account_t), compare_acct);
    rb->mutex_unlock(&save_mutex);
}

static void add_acct_file(void)
{
    char fname[MAX_PATH];
    rb->splash(HZ * 2, "Please choose the file that contains the account(s).");
    int before = next_slot;
    if(browse(fname, sizeof(fname), "/"))
    {
        int fd = rb->open(fname, O_RDONLY);
        do {
            char *uri_buf = data_buf;
            rb->memset(accounts + next_slot, 0, sizeof(struct account_t));

            accounts[next_slot].digits = 6;

            if(!rb->read_line(fd, uri_buf, URI_MAX))
                break;

            if(next_slot >= max_accts)
            {
                rb->splash(HZ * 2, "Account limit reached: some accounts not added.");
                break;
            }

            char *save;

            /* check for URI prefix */
            if(rb->strncmp(uri_buf, "otpauth://", 10))
            {
                /* see if it could be in the format name:password */
                if(rb->strchr(uri_buf, ':'))
                {
                    char *tok = rb->strtok_r(uri_buf, ":", &save);

                    if(acct_exists(tok))
                    {
                        rb->splashf(HZ * 2, "Not adding account with duplicate name `%s'!", tok);
                        continue;
                    }

                    if(!rb->strlen(tok))
                    {
                        rb->splashf(HZ * 2, "Skipping account with empty name.");
                        continue;
                    }

                    rb->strlcpy(accounts[next_slot].name, tok, sizeof(accounts[next_slot].name));

                    tok = rb->strtok_r(NULL, "", &save);
                    if(rb->strlen(tok) >= SECRET_MAX)
                        rb->splashf(HZ * 2, "Truncating secret for account `%s'", accounts[next_slot].name);
                    rb->strlcpy(accounts[next_slot].secret, tok, sizeof(accounts[next_slot].secret));
                    accounts[next_slot].type = TYPE_STATIC;
                    ++next_slot;
                }
                continue;
            }

            char *tok = rb->strtok_r(uri_buf + 10, "/", &save);
            if(!rb->strcmp(tok, "totp"))
            {
                accounts[next_slot].type = TYPE_TOTP;
                accounts[next_slot].totp_period = 30;
#if !CONFIG_RTC
                rb->splash(2 * HZ, "Skipping TOTP account (not supported).");
                continue;
#endif
            }
            else if(!rb->strcmp(tok, "hotp"))
            {
                accounts[next_slot].type = TYPE_HOTP;
                accounts[next_slot].hotp_counter = 0;
            }

            tok = rb->strtok_r(NULL, "?", &save);
            if(!tok)
                continue;

            if(acct_exists(tok))
            {
                rb->splashf(HZ * 2, "Not adding account with duplicate name `%s'!", tok);
                continue;
            }

            if(!rb->strlen(tok))
            {
                rb->splashf(HZ * 2, "Skipping account with empty name.");
                continue;
            }

            rb->strlcpy(accounts[next_slot].name, tok, sizeof(accounts[next_slot].name));

            bool have_secret = false;

            do {
                tok = rb->strtok_r(NULL, "=", &save);
                if(!tok)
                    continue;

                if(!rb->strcmp(tok, "secret"))
                {
                    if(have_secret)
                    {
                        rb->splashf(HZ * 2, "URI with multiple `secret' parameters found, skipping!");
                        goto fail;
                    }
                    have_secret = true;
                    tok = rb->strtok_r(NULL, "&", &save);
                    if((accounts[next_slot].sec_len = base32_decode(accounts[next_slot].secret, SECRET_MAX, tok)) <= 0)
                        goto fail;
                }
                else if(!rb->strcmp(tok, "counter"))
                {
                    if(accounts[next_slot].type == TYPE_TOTP)
                    {
                        rb->splash(HZ * 2, "Counter parameter specified for TOTP!? Skipping...");
                        goto fail;
                    }
                    tok = rb->strtok_r(NULL, "&", &save);
                    accounts[next_slot].hotp_counter = rb->atoi(tok);
                }
                else if(!rb->strcmp(tok, "period"))
                {
                    if(accounts[next_slot].type == TYPE_HOTP)
                    {
                        rb->splash(HZ * 2, "Period parameter specified for HOTP!? Skipping...");
                        goto fail;
                    }
                    tok = rb->strtok_r(NULL, "&", &save);
                    accounts[next_slot].totp_period = rb->atoi(tok);
                }
                else if(!rb->strcmp(tok, "digits"))
                {
                    tok = rb->strtok_r(NULL, "&", &save);
                    accounts[next_slot].digits = rb->atoi(tok);
                    if(accounts[next_slot].digits < 1 || accounts[next_slot].digits > 9)
                    {
                        rb->splashf(HZ * 2, "Digits parameter not in acceptable range, skipping.");
                        goto fail;
                    }
                }
                else
                    rb->splashf(HZ, "Unnown parameter `%s' ignored.", tok);
            } while(tok);

            if(!have_secret)
            {
                rb->splashf(HZ * 2, "URI with no `secret' parameter found, skipping!");
                goto fail;
            }

            /* wait if a background save is going on */
            rb->mutex_lock(&save_mutex);

            ++next_slot;

            rb->mutex_unlock(&save_mutex);

        fail:

            ;
        } while(1);
        rb->close(fd);
    }
    if(before == next_slot)
        rb->splash(HZ * 2, "No accounts added.");
    else
    {
        rb->splashf(HZ * 2, "Added %d account(s).", next_slot - before);
        sort_accts();
        background_save();
    }
}

static void add_acct_manual(void)
{
    if(next_slot >= max_accts)
    {
        rb->splashf(HZ * 2, "Account limit reached!");
        return;
    }
    rb->memset(accounts + next_slot, 0, sizeof(struct account_t));

    rb->splash(HZ * 1, "Enter account name:");
    if(rb->kbd_input(accounts[next_slot].name, sizeof(accounts[next_slot].name)) < 0)
        return;

    if(acct_exists(accounts[next_slot].name))
    {
        rb->splash(HZ * 2, "Duplicate account name!");
        return;
    }

    MENUITEM_STRINGLIST(type_menu, "Choose Account Type", NULL,
                        "HOTP (event-based)",
#if CONFIG_RTC
                        "TOTP (time-based)",
#endif
                        "Static Password",
                        "Cancel");

    switch(rb->do_menu(&type_menu, NULL, NULL, false))
    {
    case 0:
        accounts[next_slot].type = TYPE_HOTP;
        break;
    case 1:
#if CONFIG_RTC
            accounts[next_slot].type = TYPE_TOTP;
#else
            accounts[next_slot].type = TYPE_STATIC;
#endif
        break;
    case 2:
#if CONFIG_RTC
            accounts[next_slot].type = TYPE_STATIC;
            break;
#else
            return;
#endif
    case 3:
    default:
    case GO_TO_PREVIOUS:
        return;
    }

    char temp_buf[SECRET_MAX * 2];
    rb->memset(temp_buf, 0, sizeof(temp_buf));

    if(accounts[next_slot].type != TYPE_STATIC)
        rb->splash(HZ * 2, "Enter Base32-encoded secret:");
    else
    {
        switch(static_password_menu(accounts[next_slot].secret, SECRET_MAX))
        {
        case 0:
            break;
        case 1:
            goto done;
        case 2:
        default:
            return;
        }
    }

    if(rb->kbd_input(temp_buf, sizeof(temp_buf)) < 0)
        return;

    if(accounts[next_slot].type != TYPE_STATIC)
    {
        if((accounts[next_slot].sec_len = base32_decode(accounts[next_slot].secret, SECRET_MAX, temp_buf)) <= 0)
        {
            rb->splash(HZ * 2, "Invalid Base32 secret!");
            return;
        }
    }
    else
    {
        accounts[next_slot].sec_len = rb->strlen(temp_buf);
        if(accounts[next_slot].sec_len > SECRET_MAX)
        {
                rb->splash(HZ * 2, "Password too long!");
                return;
        }
        rb->strlcpy(accounts[next_slot].secret, temp_buf, SECRET_MAX);
        goto done;
    }

    rb->memset(temp_buf, 0, sizeof(temp_buf));

    if(accounts[next_slot].type == TYPE_HOTP)
    {
        rb->splash(HZ * 2, "Enter HOTP counter (0 is typical):");
        temp_buf[0] = '0';
    }
    else if(accounts[next_slot].type == TYPE_TOTP)
    {
        rb->splash(HZ * 2, "Enter TOTP period (30 is typical):");
        temp_buf[0] = '3';
        temp_buf[1] = '0';
    }

    if(rb->kbd_input(temp_buf, sizeof(temp_buf)) < 0)
        return;

    if(accounts[next_slot].type == TYPE_TOTP)
        accounts[next_slot].hotp_counter = rb->atoi(temp_buf);
    else
        accounts[next_slot].totp_period = rb->atoi(temp_buf);

    rb->splash(HZ * 2, "Enter digit count (6 is typical):");

    rb->memset(temp_buf, 0, sizeof(temp_buf));
    temp_buf[0] = '6';

    if(rb->kbd_input(temp_buf, sizeof(temp_buf)) < 0)
        return;

    accounts[next_slot].digits = rb->atoi(temp_buf);

    if(accounts[next_slot].digits < 1 || accounts[next_slot].digits > 9)
    {
        rb->splash(HZ, "Invalid length!");
        return;
    }

done:

    rb->mutex_lock(&save_mutex);

    ++next_slot;

    rb->mutex_unlock(&save_mutex);

    sort_accts();
    background_save();

    rb->splashf(HZ, "Success.");
}

static void add_acct(void)
{
    MENUITEM_STRINGLIST(menu, "Add Account(s)", NULL,
                        "From URI List or 'username:password' List",
                        "Manual Entry",
                        "Back");
    int sel = 0;
    bool quit = false;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            add_acct_file();
            break;
        case 1:
            add_acct_manual();
            break;
        case 2:
        default:
            quit = true;
            break;
        }
    }
}

/* core algorithm, only for OTP accounts */
static int next_code(int acct)
{
    switch(accounts[acct].type)
    {
    case TYPE_HOTP:
    {
        int ret = HOTP(accounts[acct].secret,
                       accounts[acct].sec_len,
                       accounts[acct].hotp_counter,
                       accounts[acct].digits);
        rb->mutex_lock(&save_mutex);
        ++accounts[acct].hotp_counter;
        rb->mutex_unlock(&save_mutex);
        return ret;
    }
#if CONFIG_RTC
    case TYPE_TOTP:
        return TOTP(accounts[acct].secret,
                    accounts[acct].sec_len,
                    accounts[acct].totp_period,
                    accounts[acct].digits);
#endif
    default:
        return -1;
    }
}

static void show_code(int acct)
{
    /* rockbox's printf doesn't support a variable field width afaik */
    char format_buf[64];
    switch(accounts[acct].type)
    {
    case TYPE_HOTP:
        rb->snprintf(format_buf, sizeof(format_buf), "%%0%dd", accounts[acct].digits);
        rb->splashf(0, format_buf, next_code(acct));
        background_save();
        break;
#if CONFIG_RTC
    case TYPE_TOTP:
        rb->snprintf(format_buf, sizeof(format_buf), "%%0%dd (%%ld second(s) left)", accounts[acct].digits);
        rb->splashf(0, format_buf, next_code(acct),
                    accounts[acct].totp_period - get_utc() % accounts[acct].totp_period);
        break;
#else
    case TYPE_TOTP:
        rb->splash(0, "TOTP not supported on this device!");
        break;
#endif
    case TYPE_STATIC:
        rb->splashf(0, "%s", accounts[acct].secret);
        break;
    default:
        assert(false);
        break;
    }
    rb->sleep(HZ);
    while(1)
    {
        int button = rb->button_get(true);
        if(button && !(button & BUTTON_REL))
            break;
        rb->yield();
    }

    rb->lcd_update();
}

static void gen_codes(void)
{
    acct_menu("Show Password", show_code);
}

static bool danger_confirm(void)
{
    int sel = 0;
    MENUITEM_STRINGLIST(menu, "Are you REALLY SURE?", NULL,
                        "No",
                        "No",
                        "No",
                        "No",
                        "No",
                        "No",
                        "No",
                        "Yes, DO IT", // 7
                        "No",
                        "No",
                        "No",
                        "No");

    switch(rb->do_menu(&menu, &sel, NULL, false))
    {
    case 7:
        return true;
    default:
        return false;
    }
}

static void acct_type_menu(int acct)
{
    MENUITEM_STRINGLIST(type_menu, "Choose Account Type", NULL,
                        "HOTP (event-based)",
                        "TOTP (time-based)",
                        "Static Password",
                        "Back");
    int sel = 0;
    switch(accounts[acct].type)
    {
    case TYPE_HOTP:
        break;
    case TYPE_TOTP:
        sel = 1;
        break;
    case TYPE_STATIC:
        sel = 2;
        break;
    }

    rb->mutex_lock(&save_mutex);

    if(accounts[acct].type != TYPE_STATIC)
        base32_encode(accounts[acct].secret, accounts[acct].sec_len, accounts[acct].secret, SECRET_MAX);

    bool quit = false;

    while(!quit)
    {
        switch(rb->do_menu(&type_menu, &sel, NULL, false))
        {
        case 0:
            accounts[acct].type = TYPE_HOTP;
            quit = true;
            break;
        case 1:
            accounts[acct].type = TYPE_TOTP;
            quit = true;
            break;
        case 2:
            /* base32 the secret so it's readable */
            if(accounts[acct].type != TYPE_STATIC)
                base32_encode(accounts[acct].secret, accounts[acct].sec_len, accounts[acct].secret, SECRET_MAX);
            accounts[acct].type = TYPE_STATIC;
            quit = true;
            break;
        case 3:
        default:
            quit = true;
            break;
        }
    }

    rb->mutex_unlock(&save_mutex);
}

static void edit_menu(int acct)
{
    /* HACK ALERT */
    /* three different menus, one handling logic */
    MENUITEM_STRINGLIST(menu_hotp, "Edit Account", NULL,
                        "Rename", // 0
                        "Delete", // 1
                        "Change HOTP Counter", // 2
                        "Change Digit Count", // 3
                        "Change Shared Secret", // 4
                        "Change Type", // 5
                        "Back"); // 6

    MENUITEM_STRINGLIST(menu_totp, "Edit Account", NULL,
                        "Rename", // 0
                        "Delete", // 1
                        "Change TOTP Period", // 2
                        "Change Digit Count", // 3
                        "Change Shared Secret", // 4
                        "Change Type", // 5
                        "Back"); // 6

    MENUITEM_STRINGLIST(menu_static, "Edit Account", NULL,
                        "Rename", // 0
                        "Delete", // 1
                        "Change Password", // 2
                        "Change Type", // 3
                        "Back"); // 4

    const struct menu_item_ex *menu = NULL;

    bool save = false;
    bool quit = false;
    int sel = 0;

type_change:

    switch(accounts[acct].type)
    {
    case TYPE_HOTP:
        menu = &menu_hotp;
        break;
    case TYPE_TOTP:
        menu = &menu_totp;
        break;
    case TYPE_STATIC:
        menu = &menu_static;
        break;
    default:
        break;
    }

    /* don't want to corrupt a save */
    rb->mutex_lock(&save_mutex);

    while(!quit)
    {
        switch(rb->do_menu(menu, &sel, NULL, false))
        {
        case 0: // rename
            rb->splash(HZ, "Enter new name:");
            rb->strlcpy(data_buf, accounts[acct].name, sizeof(data_buf));
            if(rb->kbd_input(data_buf, sizeof(data_buf)) < 0)
                break;
            if(acct_exists(data_buf))
            {
                rb->splash(HZ * 2, "Duplicate account name!");
                break;
            }
            rb->strlcpy(accounts[acct].name, data_buf, sizeof(accounts[acct].name));
            sort_accts();
            save = true;
            rb->splash(HZ, "Success.");
            goto done;
        case 1: // delete
            if(danger_confirm())
            {
                rb->memmove(accounts + acct, accounts + acct + 1, (next_slot - acct - 1) * sizeof(struct account_t));
                --next_slot;
                rb->splashf(HZ, "Deleted.");
                save = true;
                goto done;
            }
            else
                rb->splash(HZ, "Not confirmed.");
            break;
        case 2: // HOTP counter OR TOTP period or password
            switch(accounts[acct].type)
            {
            case TYPE_HOTP:
                rb->snprintf(data_buf, sizeof(data_buf), "%u", (unsigned int) accounts[acct].hotp_counter);
                break;
            case TYPE_TOTP:
                rb->snprintf(data_buf, sizeof(data_buf), "%d", accounts[acct].totp_period);
                break;
            case TYPE_STATIC:
                switch(static_password_menu(accounts[next_slot].secret, SECRET_MAX))
                {
                case 0:
                    rb->snprintf(data_buf, sizeof(data_buf), "%s", accounts[acct].secret);
                    break;
                case 1:
                    rb->splash(HZ, "Success.");
                    continue;
                case 2:
                default:
                    continue;
                }
            }

            if(rb->kbd_input(data_buf, sizeof(data_buf)) < 0)
                break;

            switch(accounts[acct].type)
            {
            case TYPE_TOTP:
                accounts[acct].totp_period = rb->atoi(data_buf);
                break;
            case TYPE_HOTP:
                accounts[acct].hotp_counter = rb->atoi(data_buf);
                break;
            case TYPE_STATIC:
                rb->strlcpy(accounts[acct].secret, data_buf, SECRET_MAX);
                break;
            }

            save = true;

            rb->splash(HZ, "Success.");
            break;
        case 3: // digits or type
            if(accounts[acct].type == TYPE_STATIC)
            {
                acct_type_menu(acct);
                save = true;
                rb->mutex_unlock(&save_mutex);
                goto type_change;
            }
            else
            {
                rb->snprintf(data_buf, sizeof(data_buf), "%d", accounts[acct].digits);
                if(rb->kbd_input(data_buf, sizeof(data_buf)) < 0)
                    break;

                accounts[acct].digits = rb->atoi(data_buf);

                save = true;

                rb->splash(HZ, "Success.");
            }
            break;
        case 4: // secret or back
        {
            if(accounts[acct].type == TYPE_STATIC)
            {
                quit = true;
                break;
            }
            /* save the old secret */
            size_t old_len = accounts[acct].sec_len;
            rb->memcpy(temp_sec, accounts[acct].secret, accounts[acct].sec_len);

            /* encode */
            base32_encode(accounts[acct].secret, accounts[acct].sec_len, data_buf, sizeof(data_buf));

            if(rb->kbd_input(data_buf, sizeof(data_buf)) < 0)
                break;

            int ret = base32_decode(accounts[acct].secret, sizeof(accounts[acct].secret), data_buf);
            if(ret <= 0)
            {
                rb->memcpy(accounts[acct].secret, temp_sec, SECRET_MAX);
                accounts[acct].sec_len = old_len;
                rb->splash(HZ * 2, "Invalid Base32 secret!");
                break;
            }
            accounts[acct].sec_len = ret;

            save = true;

            rb->splash(HZ, "Success.");

            break;
        }
        case 5:
            acct_type_menu(acct);
            save = true;
            rb->mutex_unlock(&save_mutex);
            goto type_change;
        case 6:
            quit = true;
            break;
        default:
            break;
        }
    }
done:

    /* done modifying */
    rb->mutex_unlock(&save_mutex);

    if(save)
        background_save();
}

static void edit_accts(void)
{
    acct_menu("Edit Account", edit_menu);
}

#if CONFIG_RTC
/* label is like this: UTC([+/-]HH:MM ...) */
static int get_time_seconds(const char *label)
{
    if(!rb->strcmp(label, "UTC"))
        return 0;

    char buf[32];

    /* copy the part after "UTC" */
    rb->strlcpy(buf, label + 3, sizeof(buf));

    char *save, *tok;

    tok = rb->strtok_r(buf, ":", &save);
    /* positive or negative: sign left */
    int hr = rb->atoi(tok);

    tok = rb->strtok_r(NULL, ": ", &save);
    int min = rb->atoi(tok);

    return 3600 * hr + 60 * min;
}

/* returns the offset in seconds associated with a time zone */
static int get_time_offs(void)
{
    MENUITEM_STRINGLIST(menu, "Select Time Zone", NULL,
                        "UTC-12:00", // 0
                        "UTC-11:00", // 1
                        "UTC-10:00 (HAST)", // 2
                        "UTC-9:30",  // 3
                        "UTC-9:00 (AKST, HADT)", // 4
                        "UTC-8:00 (PST, AKDT)", // 5
                        "UTC-7:00 (MST, PDT)", // 6
                        "UTC-6:00 (CST, MDT)", // 7
                        "UTC-5:00 (EST, CDT)", // 8
                        "UTC-4:00 (AST, EDT)", // 9
                        "UTC-3:30 (NST)", // 10
                        "UTC-3:00 (ADT)", // 11
                        "UTC-2:30 (NDT)", // 12
                        "UTC-2:00", // 13
                        "UTC-1:00", // 14
                        "UTC",      // 15
                        "UTC+1:00", // 16
                        "UTC+2:00", // 17
                        "UTC+3:00", // 18
                        "UTC+3:30", // 19
                        "UTC+4:00", // 20
                        "UTC+4:30", // 21
                        "UTC+5:00", // 22
                        "UTC+5:30", // 23
                        "UTC+5:45", // 24
                        "UTC+6:00", // 25
                        "UTC+6:30", // 26
                        "UTC+7:00", // 27
                        "UTC+8:00", // 28
                        "UTC+8:30", // 29
                        "UTC+8:45", // 30
                        "UTC+9:00", // 31
                        "UTC+9:30", // 32
                        "UTC+10:00", // 33
                        "UTC+10:30", // 34
                        "UTC+11:00", // 35
                        "UTC+12:00", // 36
                        "UTC+12:45", // 37
                        "UTC+13:00", // 38
                        "UTC+14:00", // 39
        );

    int sel = 15; // UTC
    for(unsigned int i = 0; i < ARRAYLEN(menu_); ++i)
        if(time_offs == get_time_seconds(menu_[i]))
        {
            sel = i;
            break;
        }

again:
    rb->do_menu(&menu, &sel, NULL, false);

    if(0 <= sel && sel < (int)ARRAYLEN(menu_))
    {
        /* see apps/menu.h */
        const char *label = menu_[sel];

        return get_time_seconds(label);
    }
    else
        goto again;

#if 0
    /* kept just in case menu internals change and the above code
     * breaks */
    switch(rb->do_menu(&menu, &sel, NULL, false))
    {
    case 0: case 1: case 2:
        return (sel - 12) * 3600;
    case 3:
        return -9 * 3600 - 30 * 60;
    case 4: case 5: case 6: case 7: case 8: case 9:
        return (sel - 13) * 3600;
    case 10:
        return -3 * 3600 - 30 * 60;
    case 11:
        return -3 * 3600;
    case 12:
        return -3 * 3600 - 30 * 60;
    case 13: case 14: case 15: case 16: case 17: case 18:
        return (sel - 15) * 3600;

    case 19:
        return 3 * 3600 + 30 * 60;
    case 20:
        return 4 * 3600;
    case 21:
        return 4 * 3600 + 30 * 60;
    case 22:
        return 5 * 3600;
    case 23:
        return 5 * 3600 + 30 * 60;
    case 24:
        return 5 * 3600 + 45 * 60;
    case 25:
        return 6 * 3600;
    case 26:
        return 6 * 3600 + 30 * 60;
    case 27: case 28:
        return (sel - 20) * 3600;
    case 29:
        return 8 * 3600 + 30 * 60;
    case 30:
        return 8 * 3600 + 45 * 60;
    case 31:
        return 9 * 3600;
    case 32:
        return 9 * 3600 + 30 * 60;
    case 33:
        return 10 * 3600;
    case 34:
        return 10 * 3600 + 30 * 60;
    case 35: case 36:
        return (sel - 24) * 3600;
    case 37:
        return 12 * 3600 + 45 * 60;
    case 38: case 39:
        return (sel - 25) * 3600;
    default:
        rb->splash(0, "BUG: time zone fall-through: REPORT ME!!!");
        break;
    }
    return 0;
#endif
}
#endif

#define SAVE_HOTP   (1<<0)
#define SAVE_TOTP   (1<<1)
#define SAVE_STATIC (1<<2)
#define SAVE_OTP    (SAVE_HOTP | SAVE_TOTP)
#define SAVE_ALL    (SAVE_OTP | SAVE_STATIC)

static void export_uri_list(int typemask)
{
    static char buf[MAX(MAX_PATH, SECRET_MAX * 2)];
    buf[0] = '/';
    buf[1] = '\0';
    rb->splash(HZ * 2, "Enter output filename:");
    if(rb->kbd_input(buf, sizeof(buf)) < 0)
        return;

    if(rb->file_exists(buf))
    {
        rb->splash(HZ, "File already exists!");
        return;
    }

    int fd = rb->open(buf, O_WRONLY | O_CREAT | O_TRUNC);
    if(fd < 0)
    {
        rb->splashf(HZ, "Couldn't open file.");
        return;
    }

    for(int i = 0; i < next_slot ; ++i)
    {
        if((accounts[i].type + 1) & typemask)
        {
            switch(accounts[i].type)
            {
            case TYPE_TOTP:
            case TYPE_HOTP:
                base32_encode(accounts[i].secret, accounts[i].sec_len, buf, sizeof(buf));
                rb->fdprintf(fd, "otpauth://%s/%s?secret=%s&digits=%d", accounts[i].type == TYPE_TOTP ? "totp" : "hotp",
                             accounts[i].name, buf, accounts[i].digits);

                if(accounts[i].type == TYPE_TOTP)
                    rb->fdprintf(fd, "&period=%d", accounts[i].totp_period);
            else
                rb->fdprintf(fd, "&counter=%u", (unsigned) accounts[i].hotp_counter);
                rb->fdprintf(fd, "\n");
                break;
            case TYPE_STATIC:
                rb->fdprintf(fd, "%s:%s\n", accounts[i].name, accounts[i].secret);
                break;
            }
        }
    }

    rb->close(fd);

    rb->splash(HZ, "Success.");
}

static void export_menu(void)
{
    MENUITEM_STRINGLIST(menu, "Export Accounts", NULL,
                        "To URI List (static passwords interleaved)",
                        "To URI List (only OTP accounts)",
                        "To 'username:password List' (only static passwords)",
                        "Back");

    int sel = 0;

    bool quit = false;

    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            export_uri_list(SAVE_ALL);
            break;
        case 1:
            export_uri_list(SAVE_OTP);
            break;
        case 2:
        export_uri_list(SAVE_STATIC);
        break;
        default:
            quit = true;
            break;
        }
    }
}

static void kdf_delay_menu(void)
{
    MENUITEM_STRINGLIST(menu, "Change KDF Delay", NULL,
                        "50 ms -- fastest, least secure",            // 0
                        "100 ms",                                    // 1
                        "250 ms -- default",                         // 2
                        "350 ms",                                    // 3
                        "500 ms",                                    // 4
                        "750 ms",                                    // 5
                        "1000 ms",                                   // 6
                        "1500 ms",                                   // 7
                        "2500 ms -- for the extremely paranoid",     // 8
                        "Back");
    int ticks = 0;
    while(!ticks)
    {
        switch(rb->do_menu(&menu, NULL, NULL, false))
        {
        case 0:
            ticks = 5 * HZ / 100;
            break;
        case 1:
            ticks = 10 * HZ / 100;
            break;
        case 2:
            ticks = 25 * HZ / 100;
            break;
        case 3:
            ticks = 35 * HZ / 100;
            break;
        case 4:
            ticks = 50 * HZ / 100;
            break;
        case 5:
            ticks = 75 * HZ / 100;
            break;
        case 6:
            ticks = 100 * HZ / 100;
            break;
        case 7:
            ticks = 150 * HZ / 100;
            break;
        case 8:
            ticks = 250 * HZ / 100;
            break;
        case 9:
            return;
        default:
            break;
        }
    }
    if(ticks)
    {
        rb->mutex_lock(&save_mutex);
        kdf_iters = calc_kdf_iters(ticks);
        rb->mutex_unlock(&save_mutex);
        background_save();
    }
    rb->splashf(HZ, "Using %d PBKDF2 iterations", kdf_iters);
}

/* begin using a password for encryption */
static bool change_password(void)
{
    //if(!verify_password())
    //    return false;
    char temp_pass[sizeof(enc_password)];
    char temp_pass2[sizeof(enc_password)];

    temp_pass[0] = '\0';

    rb->splash(HZ * 2, "Enter new password:");

    if(rb->kbd_input(temp_pass, sizeof(temp_pass)) < 0)
        return false;

    temp_pass2[0] = '\0';

    rb->splash(HZ * 2, "Re-enter new password:");

    if(rb->kbd_input(temp_pass2, sizeof(temp_pass2)) < 0)
        return false;

    if(rb->strcmp(temp_pass, temp_pass2))
    {
        rb->splash(HZ * 2, "Passwords do not match!");
        return false;
    }

    rb->mutex_lock(&save_mutex);

    rb->strlcpy(enc_password, temp_pass, sizeof(enc_password));

    encrypted = 1;

    rb->mutex_unlock(&save_mutex);

    background_save();

    rb->splash(HZ, "Success.");
    return true;
}

#ifdef PASSMGR_DICEWARE
static bool generate_random_passphrase(char *buf, size_t buflen)
{
    char key[20];

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif

    gather_entropy(key, 20);

    /* give the user time to stop pressing keys */
    rb->splash(HZ * 2, "Done.");

    rb->button_clear_queue();

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif

    assert(DICEWARE_WORDS < 160/16);

    uint16_t *ptr;

    for(int i = 0; i < 10; ++i)
    {
        /* generate a passphrase until the user decides on one or we try 10 times */

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(true);
#endif

        ptr = (uint16_t*)key;
        buf[0] = '\0';
        rb->splash(0, "Generating...");
        for(int i = 0; i < DICEWARE_WORDS; ++i)
        {
            uint16_t rnd;
            /* rehash until we get a good value */
            /* this smells awfully like bitcoin mining */
            do {
                rnd = *ptr;
                long timestamp = *rb->current_tick;
                hmac_sha1(&timestamp, sizeof(long), key, 20, key);
            } while(rnd >= word_list_len);

            rb->strlcat(buf, word_list[rnd], buflen);
            rb->strlcat(buf, " ", buflen);
            rb->yield();
        }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
        rb->cpu_boost(false);
#endif

        rb->backlight_set_timeout(0); /* no timeout */

        struct text_message prompt = { (const char*[]) { "Your generated passphrase is:", buf, "Is this OK?" }, 3 };
        enum yesno_res response = rb->gui_syncyesno_run(&prompt, NULL, NULL);
        if(response == YESNO_NO)
        {
            long timestamp = *rb->current_tick;

            /* mix it with an HMAC */
            hmac_sha1(&timestamp, sizeof(long), key, 20, key);
        }
        else
        {
            struct text_message prompt2 = { (const char*[]) { "Again, your passphrase is:", buf, "Please take time to commit this to memory.", "If you forget it, your data will be irretrievably lost!", "Continue?"}, 5 };
            enum yesno_res response = rb->gui_syncyesno_run(&prompt2, NULL, NULL);
            if(response == YESNO_YES)
                return true;
        }
    }

    return false;
}

/* use a passphrase */
static bool change_passphrase(void)
{
    MENUITEM_STRINGLIST(mode_menu, "Choose Passphase Generation Method", NULL,
                        "Random Generation",
                        "Manual Entry (not recommended)",
                        "Cancel");

    bool done = false;
    while(!done)
    {
        switch(rb->do_menu(&mode_menu, NULL, NULL, false))
        {
        case 0:
        /* we need the data buffer */
            rb->mutex_lock(&save_mutex);
            if(!generate_random_passphrase(data_buf, sizeof(data_buf)))
            {
                rb->mutex_unlock(&save_mutex);
                return false;
            }
            rb->mutex_unlock(&save_mutex);
            done = true;
            break;
        case 1:
            rb->splash(HZ, "Enter new passphrase:");
            rb->mutex_lock(&save_mutex);
            if(read_diceware_passphrase(data_buf, sizeof(data_buf)))
            {
                /* failure */
                rb->mutex_unlock(&save_mutex);
                return false;
            }
            rb->mutex_unlock(&save_mutex);
            done = true;
            break;
        default:
            return false;
        }
    }

    rb->mutex_lock(&save_mutex);

    rb->strlcpy(enc_password, data_buf, sizeof(enc_password));
    encrypted = 2;

    rb->mutex_unlock(&save_mutex);

    background_save();

    rb->splash(HZ, "Success.");

    return true;
}
#endif /* #ifdef PASSMGR_DICEWARE */

static bool disable_encryption(void)
{
    //if(!verify_password())
    //    return false;
    rb->mutex_lock(&save_mutex);
    encrypted = 0;
    rb->mutex_unlock(&save_mutex);
    background_save();
    rb->splash(HZ, "Success.");
    return true;
}

static void encrypt_menu(void)
{
    /* 3 states for the menu */
    MENUITEM_STRINGLIST(encrypt_menu_0, "Encryption", NULL,
                        "Enable",
                        "Back");

    MENUITEM_STRINGLIST(encrypt_menu_1 , "Encryption", NULL,
                        "Change Password",
                        "Change KDF Delay",
#ifdef PASSMGR_DICEWARE
                        "Use a Passphrase",
#endif
                        "Disable",
                        "Back");

#ifdef PASSMGR_DICEWARE
    MENUITEM_STRINGLIST(encrypt_menu_2 , "Encryption", NULL,
                        "Change Passphrase",
                        "Change KDF Delay",
                        "Use a Password",
                        "Disable",
                        "Back");
#endif

    const struct menu_item_ex *menus[] = { &encrypt_menu_0,
                                           &encrypt_menu_1,
#ifdef PASSMGR_DICEWARE
                                           &encrypt_menu_2,
#endif
    };

    const struct menu_item_ex *menu;
state_change:
    menu = menus[(int)encrypted];

    bool done = false;

    while(!done)
    {
        int sel = rb->do_menu(menu, NULL, NULL, false);
        switch(encrypted)
        {
        case 0: /* disabled */
        {
            switch(sel)
            {
            case 0: /* enable, change type */
                change_password();
                goto state_change;
            case 1: /* back */
            default:
                done = true;
                break;
            }
            break;
        }
        case 1: /* using a password */
        {
            switch(sel)
            {
            case 0: /* change password */
                change_password();
                break; /* no state change */
            case 1: /* KDF delay */
                kdf_delay_menu();
                break;
#ifdef PASSMGR_DICEWARE
            case 2: /* use passphrase */
                change_passphrase();
                goto state_change;
            case 3: /* disable */
                disable_encryption();
                goto state_change;
            case 4: /* back */
                done = true;
                break;
#else
            case 2: /* disable */
                disable_encryption();
                goto state_change;
            case 3: /* back */
                done = true;
                break;
#endif
            default:
                break;
            }
            break;
        }
#ifdef PASSMGR_DICEWARE
        case 2: /* we have a passphrase configured */
        {
            switch(sel)
            {
            case 0: /* change passphrase */
                change_passphrase();
                break;
            case 1: /* KDF delay */
                kdf_delay_menu();
                break;
            case 2: /* use a password */
                change_password();
                goto state_change;
            case 3: /* disable */
                disable_encryption();
                goto state_change;
            case 4:
                done = true;
                break;
            default:
                break;
            }
        }
#endif
        default:
            break;
        }
    }
}

static void adv_menu(void)
{
    MENUITEM_STRINGLIST(menu, "Advanced", NULL,
                        "Edit Account",
                        "Export Accounts",
                        "Encryption",
                        "Delete ALL Accounts",
#if CONFIG_RTC
                        "Select Time Zone",
#endif
                        "Back");

    bool quit = false;
    int sel = 0;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            edit_accts();
            break;
        case 1:
            export_menu();
            break;
        case 2:
        {
            encrypt_menu();
            break;
        }
        case 3:
            if(danger_confirm())
            {
                rb->mutex_lock(&save_mutex);
                next_slot = 0;
                rb->mutex_unlock(&save_mutex);
                save_accts();
                rb->splash(HZ, "It is done, my master.");
            }
            else
                rb->splash(HZ, "Not confirmed.");
            break;
#if CONFIG_RTC
        case 4:
        {
            int old_offs = time_offs;
            rb->mutex_lock(&save_mutex);
            time_offs = get_time_offs();
            rb->mutex_unlock(&save_mutex);
            if(time_offs != old_offs)
                background_save();
            break;
        }
        case 5:
#else
        case 4:
#endif
            quit = 1;
            break;
        default:
            break;
        }
    }
}

static char *help_text[] = { "Password Manager", "",
                             "",
                             "Introduction", "",
                             "This", "plugin", "allows", "you", "to", "generate", "one-time", "passwords", "as", "a", "second", "factor", "of", "authentication", "for", "online", "services", "which", "support", "it,", "such", "as", "GitHub", "and", "Google.",
                             "This", "plugin", "supports", "both", "counter-based", "(HOTP),", "and", "time-based", "(TOTP)", "password", "schemes.",
                             "It", "also", "supports", "storing", "static", "passwords", "securely.",
                             "",
                             "",
                             "Time Zone Configuration", "",
                             "On", "the", "first", "run", "of", "the", "plugin,", "you", "are", "asked", "for", "the", "time", "zone", "to", "which", "your", "system", "clock", "is", "set.",
                             "If", "you", "need", "to", "change", "this", "setting", "later,", "it", "is", "available", "under", "the", "'Advanced'", "menu", "option.",
                             "",
                             "",
                             "Account Setup", "",
                             "To", "add", "a", "new", "account,", "choose", "the", "'Add", "Account(s)'", "menu", "option.",
                             "There", "are", "two", "ways", "to", "add", "an", "account,", "either", "from", "a", "file", "containing", "account", "information", "in", "URI", "format,", "or", "manual", "entry.",
                             "",
                             "",
                             "URI Import", "",
                             "This", "method", "of", "adding", "an", "account", "reads", "a", "list", "of", "URIs", "from", "a", "file.",
                             "It", "expects", "each", "URI", "to", "be", "on", "a", "line", "by", "itself", "in", "the", "following", "format:", "",
                             "",
                             "otpauth://[hotp", "OR", "totp]/[account", "name]?secret=[Base32", "secret][&counter=X][&period=X][&digits=X]", "",
                             "",
                             "An", "example", "is", "shown", "below,", "provisioning", "a", "TOTP", "key", "for", "an", "account", "called", "``bob'':", "",
                             "",
                             "otpauth://totp/bob?secret=JBSWY3DPEHPK3PXP", "",
                             "",
                             "Any", "other", "URI", "options", "are", "not", "supported", "and", "will", "be", "ignored.",
                             "",
                             "Most", "services", "will", "provide", "a", "scannable", "QR", "code", "that", "encodes", "a", "OTP", "URI.",
                             "In", "order", "to", "use", "those,", "first", "scan", "the", "QR", "code", "separately", "and", "save", "the", "URI", "to", "a", "file", "on", "your", "device.",
                             "If", "necessary,", "rewrite", "the", "URI", "so", "it", "is", "in", "the", "format", "shown", "above.",
                             "For", "example,", "GitHub's", "URI", "has", "a", "slash", "after", "the", "provider.",
                             "In", "order", "for", "this", "URI", "to", "be", "properly", "parsed,", "you", "must", "rewrite", "the", "account", "name", "so", "that", "it", "does", "not", "contain", "a", "slash.",
                             "",
                             "",
                             "Manual Import", "",
                             "If", "direct", "URI", "import", "is", "not", "possible,", "the", "plugin", "supports", "the", "manual", "entry", "of", "data", "associated", "with", "an", "account.",
                             "After", "you", "select", "the", "'Manual", "Entry'", "option,", "it", "will", "prompt", "you", "for", "an", "account", "name.",
                             "You", "may", "type", "anything", "you", "wish,", "but", "it", "should", "be", "memorable.",
                             "It", "will", "then", "prompt", "you", "for", "the", "Base32-encoded", "secret.",
                             "Most", "services", "will", "provide", "this", "to", "you", "directly,", "but", "some", "may", "only", "provide", "you", "with", "a", "QR", "code.",
                             "In", "these", "cases,", "you", "must", "scan", "the", "QR", "code", "separately,", "and", "then", "enter", "the", "string", "following", "the", "'secret='", "parameter", "on", "your", "Rockbox", "device", "manually.",
                             "",
                             "On", "devices", "with", "a", "real-time", "clock,", "the", "plugin", "will", "ask", "whether", "the", "account", "is", "a", "time-based", "account", "(TOTP).",
                             "If", "you", "answer", "'yes'", "to", "this", "question,", "it", "will", "ask", "for", "further", "information", "regarding", "the", "account.",
                             "Usually", "it", "is", "safe", "to", "accept", "the", "defaults", "here.",
                             "However,", "if", "your", "device", "lacks", "a", "real-time", "clock,", "the", "plugin's", "functionality", "will", "be", "restricted", "to", "HMAC-based", "(HOTP)", "accounts", "only.",
                             "If", "this", "is", "the", "case,", "the", "plugin", "will", "prompt", "you", "for", "information", "regarding", "the", "HOTP", "setup.",
                             "Again,", "it", "is", "usually", "safe", "to", "accept", "the", "defaults.",
                             "",
                             "",
                             "Account Export", "",
                             "This", "plugin", "allows", "you", "to", "export", "account", "data", "to", "a", "file", "for", "backup", "and", "transfer", "purposes.",
                             "This", "option", "is", "located", "under", "the", "'Advanced'", "menu.",
                             "It", "will", "prompt", "for", "for", "a", "filename,", "and", "will", "write", "all", "your", "account", "data", "to", "the", "specified", "file.",
                             "This", "file", "can", "be", "imported", "by", "this", "plugin", "using", "the", "'From", "URI", "List'", "option", "when", "importing.",
                             "Please", "note", "that", "you", "should", "not", "attempt", "to", "copy", "the", "'passmgr.dat'", "from", "the", ".rockbox", "directory", "to", "another", "device.",
                             "",
                             "",
                             "Encryption", "",
                             "This", "plugin", "supports", "the", "optional", "encryption", "of", "account", "data", "while", "stored", "on", "disk.",
                             "This", "feature", "is", "located", "under", "the", "'Advanced'", "menu", "option.",
                             "Upon", "enabling", "this", "feature,", "you", "must", "enter", "an", "encryption", "password", "that", "will", "need", "to", "be", "entered", "each", "time", "the", "plugin", "starts", "up.",
                             "It", "is", "recommended", "that", "you", "use", "a", "strong,", "alphanumeric", "password", "of", "at", "least", "8", "characters", "in", "order", "to", "frustrate", "attempts", "to", "guess", "the", "password.",
                             "Be", "sure", "not", "to", "forget", "this", "password.",
                             "In", "the", "event", "that", "the", "password", "is", "lost,", "it", "is", "nearly", "impossible", "to", "recover", "your", "account", "data.",
                             "",
                             "",
                             "Implementation Details", "",
                             "Account", "data", "is", "encrypted", "with", "128-bit", "AES", "encryption", "in", "counter", "mode.",
                             "The", "key", "is", "derived", "from", "the", "your", "password", "and", "a", "nonce", "by", "using", "PBKDF2-HMAC-SHA1,", "with", "a", "variable", "number", "of", "iterations,", "calibrated", "by", "default", "to", "take", "250", "milliseconds.",
                             "This", "parameter", "can", "be", "adjusted", "using", "the", "'Change", "KDF", "Delay'", "option", "under", "the", "'Encryption'", "submenu.",
                             "The", "nonce", "is", "generated", "from", "the", "system's", "current", "tick", "and", "the", "real-time", "clock,", "if", "available,", "making", "collision", "unlikely.",
                             "Some", "later-model", "iPods", "have", "a", "hardware", "AES", "core", "with", "a", "hardcoded,", "device-specific", "key", "that", "cannot", "easily", "be", "extracted.",
                             "When", "available,", "the", "device-specific", "key", "is", "used", "to", "encrypt", "the", "actual", "encryption", "key,", "tying", "the", "ciphertext", "to", "the", "device,", "making", "a", "brute-force", "attack", "more", "difficult.",
                             "One", "should", "note", "that", "this", "does", "not", "rely", "completely", "rely", "on", "the", "hardware", "encryption", "key,", "it", "merely", "utilizes", "it", "as", "part", "of", "defense", "in", "depth.",
                             "",
                             "",
                             "Troubleshooting", "",
                             "If", "time-based", "passwords", "and", "not", "working", "properly,", "ensure", "that", "your", "system", "clock", "is", "accurate", "to", "within", "30", "seconds", "of", "the", "authenticating", "server's", "clock,", "and", "that", "the", "proper", "time", "zone", "is", "configured", "within", "the", "plugin.",
                             "Be", "sure", "to", "account", "for", "Daylight", "Savings", "Time,", "if", "applicable.",
                             "",
                             "",
                             "Supported Features", "",
#if !CONFIG_RTC
                             "This", "device", "lacks", "a", "real-time", "clock,", "and", "thus", "time-based", "(TOTP)", "passwords", "are", "not", "supported.",
                             "",
#endif
#if CONFIG_CPU == S5L8702 && !defined(SIMULATOR)
                             "This", "device", "has", "a", "hardware", "AES", "core", "that", "will", "be", "used", "to", "further", "protect", "your", "data", "by", "tying", "it", "to", "this", "device.",
                             "",
#else
                             "This", "device", "does", "not", "have", "a", "hardware", "AES", "core.",
                             "The", "security", "of", "the", "encryption", "thus", "relies", "solely", "on", "your", "password.",
                             "",
#endif
#ifdef USB_ENABLE_HID
                             "This", "device", "has", "the", "ability", "to", "type", "passwords", "directly", "to", "a", "host", "computer", "over", "the", "USB", "connection.",
                             "",
#endif
};

struct style_text style[] = {
    { 0, TEXT_CENTER | TEXT_UNDERLINE },
    { 3, C_RED },
    { 50, C_RED },
    { 91, C_RED },
    { 127, C_RED },
    { 280, C_RED },
    { 468, C_RED },
    { 548, C_RED },
    { 644, C_RED },
    { 787, C_RED },
    { 835, C_RED },
    LAST_STYLE_ITEM
};


/* displays the help text */
static void show_help(void)
{

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_WHITE);
    rb->lcd_set_background(LCD_BLACK);
#endif

#ifdef HAVE_LCD_BITMAP
    rb->lcd_setfont(FONT_UI);
#endif
    display_text(ARRAYLEN(help_text), help_text, style, NULL, true);
}

#ifdef USB_ENABLE_HID

#define FORCE_EXEC_THRES (HZ/3)
#define TYPE_DELAY (HZ / 25)

static bool wait_for_usb(void)
{
    if(!rb->usb_inserted())
    {
        /* wait for a USB connection */

        rb->splash(0, "Waiting for USB, hold any button to abort...");

        int oldbutton = 0;
        int ticks_held = 0;
        long last_tick = 0;
        while(1)
        {
            int button = rb->button_get(true);
            if(button == SYS_USB_CONNECTED)
            {
                break;
            }
            else if(button)
            {
                /* check if a key is being held down */

                if(oldbutton == 0)
                {
                    oldbutton = button;

                    ticks_held = 0;
                    last_tick = *rb->current_tick;
                }
                else if(button == oldbutton || button == (oldbutton | BUTTON_REPEAT))
                {
                    int dt = *rb->current_tick - last_tick;
                    if(dt)
                    {
                        ticks_held += dt;
                        last_tick = *rb->current_tick;
                        if(ticks_held >= FORCE_EXEC_THRES)
                            return false;
                    }
                }
            }
        }

        /* wait a bit to let the host recognize us... */
        rb->sleep(HZ / 2);
    }
    return true;
}

static void send(int status)
{
    rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, status);
}

/* Rockbox's HID driver supports up to 4 keys simultaneously, 1 in each byte */

static void add_key(int *keystate, unsigned *nkeys, int newkey)
{
    *keystate = (*keystate << 8) | newkey;
    if(nkeys)
        (*nkeys)++;
}

struct char_mapping {
    char c;
    int key;
};

static struct char_mapping shift_tab[] = {
    { '~', HID_KEYBOARD_BACKTICK },
    { '!', HID_KEYBOARD_1 },
    { '@', HID_KEYBOARD_2 },
    { '#', HID_KEYBOARD_3 },
    { '$', HID_KEYBOARD_4 },
    { '%', HID_KEYBOARD_5 },
    { '^', HID_KEYBOARD_6 },
    { '&', HID_KEYBOARD_7 },
    { '*', HID_KEYBOARD_8 },
    { '(', HID_KEYBOARD_9 },
    { ')', HID_KEYBOARD_0 },
    { '_', HID_KEYBOARD_HYPHEN },
    { '+', HID_KEYBOARD_EQUAL_SIGN },
    { '}', HID_KEYBOARD_RIGHT_BRACKET },
    { '{', HID_KEYBOARD_LEFT_BRACKET },
    { '|', HID_KEYBOARD_BACKSLASH },
    { '"', HID_KEYBOARD_QUOTE },
    { ':', HID_KEYBOARD_SEMICOLON },
    { '?', HID_KEYBOARD_SLASH },
    { '>', HID_KEYBOARD_DOT },
    { '<', HID_KEYBOARD_COMMA },
};

static struct char_mapping char_tab[] = {
    { ' ', HID_KEYBOARD_SPACEBAR },
    { '`', HID_KEYBOARD_BACKTICK },
    { '-', HID_KEYBOARD_HYPHEN },
    { '=', HID_KEYBOARD_EQUAL_SIGN },
    { '[', HID_KEYBOARD_LEFT_BRACKET },
    { ']', HID_KEYBOARD_RIGHT_BRACKET },
    { '\\', HID_KEYBOARD_BACKSLASH },
    { '\'', HID_KEYBOARD_QUOTE },
    { ';', HID_KEYBOARD_SEMICOLON },
    { '/', HID_KEYBOARD_SLASH },
    { ',', HID_KEYBOARD_COMMA },
    { '.', HID_KEYBOARD_DOT },
    { '\t',HID_KEYBOARD_TAB },
};

static void add_char(int *keystate, unsigned *nkeys, char c)
{
    (void) keystate; (void) nkeys; (void) c;
    if('a' <= c && c <= 'z')
    {
        add_key(keystate, nkeys, c - 'a' + HID_KEYBOARD_A);
    }
    else if('A' <= c && c <= 'Z')
    {
        add_key(keystate, nkeys, HID_KEYBOARD_LEFT_SHIFT);
        add_key(keystate, nkeys, c - 'A' + HID_KEYBOARD_A);
    }
    else if('0' <= c && c <= '9')
    {
        if(c == '0')
            add_key(keystate, nkeys, HID_KEYPAD_0_AND_INSERT);
        else
            add_key(keystate, nkeys, c - '1' + HID_KEYPAD_1_AND_END);
    }
    else
    {
        /* search the character table */
        for(unsigned int i = 0; i < ARRAYLEN(char_tab); ++i)
        {
            if(char_tab[i].c == c)
            {
                add_key(keystate, nkeys, char_tab[i].key);
                return;
            }
        }

        /* search the shift-mapping table */
        for(unsigned int i = 0; i < ARRAYLEN(shift_tab); ++i)
        {
            if(shift_tab[i].c == c)
            {
                add_key(keystate, nkeys, HID_KEYBOARD_LEFT_SHIFT);
                add_key(keystate, nkeys, shift_tab[i].key);
                return;
            }
        }

        rb->splashf(HZ, "WARNING: could not type character '%c'!", c);
    }
}

static void send_string(const char *str)
{
    while(*str)
    {
        int string_state = 0;
        if(!*str)
            break;
        add_char(&string_state, NULL, *str);

        send(string_state);

        ++str;

        rb->sleep(TYPE_DELAY);
    }
}

static bool enable_numlock(void)
{
    /* check numlock status */
    bool change_numlock = !(rb->usb_hid_leds() & 0x1);
    if(change_numlock)
        rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, HID_KEYPAD_NUM_LOCK_AND_CLEAR);
    return change_numlock;
}

static void type_code(int acct)
{
    if(!wait_for_usb())
        return;

    rb->splash(0, "Typing...");

    bool change_numlock = enable_numlock();

    switch(accounts[acct].type)
    {
    case TYPE_HOTP:
    case TYPE_TOTP:
    {
        int code = next_code(acct);

        /* hackery to get around the lack of %*d support */
        char fmt_buf[64], buf[64];

        rb->snprintf(fmt_buf, sizeof(fmt_buf), "%%0%dd", accounts[acct].digits);
        rb->snprintf(buf, sizeof(buf), fmt_buf, code);

        char *ptr = buf;

        while(*ptr)
        {
            char c = *ptr++;
            if(c == '0')
                rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, HID_KEYPAD_0_AND_INSERT);
            else
                rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, c - '1'  + HID_KEYPAD_1_AND_END);
            rb->sleep(TYPE_DELAY);
        }
        if(accounts[acct].type == TYPE_HOTP)
            background_save();
        break;
    }
    case TYPE_STATIC:
        send_string(accounts[acct].secret);
        break;
    default:
        break;
    }

    rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, HID_KEYBOARD_RETURN);

    if(change_numlock)
        rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, HID_KEYPAD_NUM_LOCK_AND_CLEAR);

    rb->splash(0, "Done.");

    /* wait a while to prevent accidental code generation */
    rb->sleep(HZ / 2);
    while(1)
    {
        int button = rb->button_get(true);
        if(button && !(button & BUTTON_REL))
            break;
        rb->yield();
    }

    rb->lcd_update();
}

static void type_codes(void)
{
    if(!rb->global_settings->usb_hid)
    {
        rb->splashf(HZ * 4, "Please enable USB HID in the system settings.");
    }
    acct_menu("Type Password", type_code);
}
#endif

/* based on keybox */

static const char* list_cb(int selected_item, void *data,
                           char *buffer, size_t buffer_len)
{
    (void) data;
    rb->snprintf(buffer, buffer_len, "%s", accounts[selected_item].name);
    return buffer;
}

static void acct_menu(char *title, void (*cb)(int acct))
{
    struct gui_synclist list;

    rb->gui_synclist_init(&list, &list_cb, NULL, false, 1, NULL);
    rb->gui_synclist_set_title(&list, title, NOICON);
    rb->gui_synclist_set_icon_callback(&list, NULL);
    rb->gui_synclist_set_nb_items(&list, next_slot);
    rb->gui_synclist_limit_scroll(&list, false);
    rb->gui_synclist_select_item(&list, 0);

    bool done = false;

    while (!done)
    {
        rb->gui_synclist_draw(&list);
        int button = rb->get_action(CONTEXT_LIST, TIMEOUT_BLOCK);

#ifdef USB_ENABLE_HID
        /* ignore USB connections when in USB mode */
        if(cb != type_code || button != SYS_USB_CONNECTED)
#endif
            if (rb->gui_synclist_do_button(&list, &button, LIST_WRAP_ON))
                continue;

        switch (button)
        {
        case ACTION_STD_OK:
            cb(rb->gui_synclist_get_sel_pos(&list));
            rb->gui_synclist_set_nb_items(&list, next_slot);
            if(rb->gui_synclist_get_sel_pos(&list) >= next_slot)
                rb->gui_synclist_select_item(&list, next_slot - 1);
            break;
        case ACTION_STD_CONTEXT:
            if(cb != edit_menu)
                edit_menu(rb->gui_synclist_get_sel_pos(&list));
            rb->gui_synclist_set_nb_items(&list, next_slot);
            if(rb->gui_synclist_get_sel_pos(&list) >= next_slot)
                rb->gui_synclist_select_item(&list, next_slot - 1);
            break;
        case ACTION_STD_CANCEL:
            done = true;
            break;
        }
        rb->yield();
    }

    return;

    rb->lcd_clear_display();
    /* native menus don't seem to support dynamic names easily, so we
     * roll our own */
    static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };
    int idx = 0;
    if(next_slot > 0)
    {
        rb->lcd_puts(0, 0, title);
        rb->lcd_putsf(0, 1, "%s", accounts[0].name);
        rb->lcd_update();
    }
    else
    {
        rb->splash(HZ * 2, "No accounts configured!");
        return;
    }
    while(1)
    {
        int button = pluginlib_getaction(-1, plugin_contexts, ARRAYLEN(plugin_contexts));
        switch(button)
        {
        case PLA_LEFT:
            --idx;
            if(idx < 0)
                idx = next_slot - 1;
            break;
        case PLA_RIGHT:
            ++idx;
            if(idx >= next_slot)
                idx = 0;
            break;
        case PLA_SELECT:
            cb(idx);
            if(idx >= next_slot)
                idx = 0;
            if(next_slot == 0)
                return;
            break;
        case PLA_UP:
        case PLA_CANCEL:
        case PLA_EXIT:
            return;
        default:
#ifdef USB_ENABLE_HID
            if(cb != type_code)
#endif
                exit_on_usb(button);
            break;
        }
        rb->lcd_clear_display();
        rb->lcd_puts(0, 0, title);
        rb->lcd_putsf(0, 1, "%s", accounts[idx].name);
        rb->lcd_update();
        rb->yield();
    }
}

static bool self_check(void)
{
    /* RFC 4226 */
    if(HOTP("12345678901234567890", rb->strlen("12345678901234567890"), 1, 6) != 287082)
        return false;

    /* do a 2-byte KDF just to check that I didn't break TOO many things :P */

    unsigned char out[2];
    char tmp[4 + 4];

    PBKDF2("password", 8, "salt", 4, 2, out, 2, tmp);

    if(out[0] != 0xea || out[1] != 0x6c)
        return false;

    return true;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    (void)parameter;

    if(!self_check())
    {
        rb->splash(HZ * 4, "Self-test failed! REPORT ME!");
        return PLUGIN_ERROR;
    }

    size_t bufsz;
    accounts = rb->plugin_get_buffer(&bufsz);
    max_accts = bufsz / sizeof(struct account_t);

    atexit(erase_sensitive_info);

    if(!read_accts())
    {
#if CONFIG_RTC
        /* first-run config */
        time_offs = get_time_offs();
#endif
        kdf_iters = calc_kdf_iters(KDF_DEFAULT);
    }

    /* initialize background saving thread */
    rb->mutex_init(&save_mutex);
    background_id = rb->create_thread(background_thread, background_stack,
                                      sizeof(background_stack), 0,
                                      "background_save" IF_PRIO(, PRIORITY_BACKGROUND) IF_COP(, COP));

    MENUITEM_STRINGLIST(menu, "Password Manager", NULL,
                        "Show Password", // 0
#ifdef USB_ENABLE_HID
                        "Type Password", // 1
#endif
                        "Add Account(s)", // 1,2
                        "Help", // 2,3
                        "Advanced", // 3,4
                        "Quit"); // 4,5

    bool quit = false;
    int sel = 0;
    while(!quit)
    {
        switch(rb->do_menu(&menu, &sel, NULL, false))
        {
        case 0:
            gen_codes();
            break;
#ifdef USB_ENABLE_HID
        case 1:
            type_codes();
            break;
        case 2:
            add_acct();
            break;
        case 3:
            show_help();
            break;
        case 4:
            adv_menu();
            break;
        case 5:
            quit = 1;
            break;
#else
        case 1:
            add_acct();
            break;
        case 2:
            show_help();
            break;
        case 3:
            adv_menu();
            break;
        case 4:
            quit = 1;
            break;
#endif
        default:
            break;
        }
    }

    rb->mutex_lock(&save_mutex); // make sure we aren't saving

    /* kill the background thread */
    kill_background = true;
    if(background_id >= 0)
        rb->thread_wait(background_id);

    /* save to disk */
    save_accts();

    /* tell Rockbox that we have completed successfully */
    return PLUGIN_OK;
}

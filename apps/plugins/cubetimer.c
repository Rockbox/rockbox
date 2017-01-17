/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Franklin Wei
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

#include "plugin.h"

#include "lib/helper.h"
#include "lib/sha1.h"

#define ENTROPY_FILE PLUGIN_APPS_DATA_DIR "/cubetimer.rnd"

#define YIELD_INTERVAL 2500

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

/* generate a desired number of random bits */
/* note that len must be a multiple of sizeof(long) and >= 20 */
static void gather_entropy(void *out, size_t len)
{
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
    for(int keys = 0; keys < 10; ++keys)
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

        /* finally mix it all up with an HMAC */
        hmac_sha1(&timestamp, sizeof(long), ptr, 20, ptr);
        rb->yield();
    }

    /* as a final step we use PBKDF2 to stretch the 20 bytes we have
     * to the desired length */
    long salt = *rb->current_tick;
    char tmp[sizeof(salt) + 4];
    PBKDF2(buf, 20, &salt, sizeof(salt), 1, out, len, tmp);
}

static char   entropy_pool[20];
static size_t entropy_index = 0;

void load_entropy(void)
{
    if(rb->file_exists(ENTROPY_FILE))
    {
        int fd = rb->open(ENTROPY_FILE, O_RDONLY);
        if(fd < 0)
            goto fail;
        if(rb->read(fd, entropy_pool, sizeof(entropy_pool)) != sizeof(entropy_pool))
        {
            rb->close(fd);
            goto fail;
        }
        rb->close(fd);
        return;
    }
    else
    {
    fail:
        gather_entropy(entropy_pool, sizeof(entropy_pool));
        return;
    }
}

/* this is all horrid code security-wise and probably speed-wise too,
 * but for this application it'll be just fine */

static void stir_entropy(void)
{
    char hash[20];
    sha1_buffer(entropy_pool, sizeof(entropy_pool), hash);
    rb->memcpy(entropy_pool, hash, sizeof(entropy_pool));
}

static void get_random(unsigned char *out, size_t len)
{
    while(len-- > 0)
    {
        *out++ = entropy_pool[entropy_index++];
        if(entropy_index >= sizeof(entropy_pool))
        {
            stir_entropy();
            entropy_index = 0;
        }
    }
}

static unsigned char get_random_byte(void)
{
    unsigned char ret;
    get_random(&ret, 1);
    return ret;
}

/* return a byte from 0 to max - 1, inclusive */
static unsigned char get_random_upto(unsigned char max)
{
    unsigned char ret;
    do {
        ret = get_random_byte();
    } while(ret >= max);
    return ret;
}

static void save_entropy(void)
{
    /* don't save used bytes */
    stir_entropy();
    int fd = rb->open(ENTROPY_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd < 0)
    {
        rb->splash(HZ * 2, "Couldn't save entropy!");
        return;
    }

    rb->write(fd, entropy_pool, sizeof(entropy_pool));
    rb->close(fd);
}

#define SHUFFLE_LEN 25

static void show_shuffle(void)
{
    static struct face_t {
        char name;
        int opp;
    } faces[] = { {'L', 1},
                  {'R', 0},
                  {'U', 3},
                  {'D', 2},
                  {'F', 5},
                  {'B', 4},
    };
    static char turns[] = { '\0', '2', '\'' };
    char seq[SHUFFLE_LEN * 3];
    char *ptr = seq;
    int lastface = -1, lastopp = -1;
    for(int i = 0; i < SHUFFLE_LEN; ++i)
    {
        int face;
    again:
        face = get_random_upto(ARRAYLEN(faces));
        if(face == lastface || face == lastopp)
            goto again;
        lastface = face;
        lastopp = faces[face].opp;
        int turn = get_random_upto(sizeof(turns));
        *ptr++ = faces[face].name;
        if(turns[turn])
            *ptr++ = turns[turn];
        if(i != SHUFFLE_LEN - 1)
            *ptr++ = ' ';
        else
            *ptr++ = '\0';
    }
    backlight_ignore_timeout();
    rb->splash(0, seq);
    rb->button_get(true);
    backlight_use_settings();
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    /* if you don't use the parameter, you can do like
       this to avoid the compiler warning about it */
    (void)parameter;

    load_entropy();

    MENUITEM_STRINGLIST(menu, "Cube Timer Menu", NULL,
                        "Show Shuffle",
                        "Time Solve",
                        "Quit");

    bool done = false;

    while(!done)
    {
        switch(rb->do_menu(&menu, NULL, NULL, false))
        {
        case 0:
            show_shuffle();
            break;
        case 1:
            break;
        case 2:
            done = true;
            break;
        }
    }

    save_entropy();

    return PLUGIN_OK;
}

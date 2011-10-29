/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Amaury Pouly
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
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include "misc.h"

bool g_debug = false;

/**
 * Misc
 */

char *s_getenv(const char *name)
{
    char *s = getenv(name);
    return s ? s : "";
}

void generate_random_data(void *buf, size_t sz)
{
    FILE *rand_fd = fopen("/dev/urandom", "rb");
    if(rand_fd == NULL)
        bugp("failed to open /dev/urandom");
    if(fread(buf, 1, sz, rand_fd) != sz)
        bugp("failed to read /dev/urandom");
    fclose(rand_fd);
}

void *xmalloc(size_t s)
{
    void * r = malloc(s);
    if(!r) bugp("malloc");
    return r;
}

int convxdigit(char digit, byte *val)
{
    if(digit >= '0' && digit <= '9')
    {
        *val = digit - '0';
        return 0;
    }
    else if(digit >= 'A' && digit <= 'F')
    {
        *val = digit - 'A' + 10;
        return 0;
    }
    else if(digit >= 'a' && digit <= 'f')
    {
        *val = digit - 'a' + 10;
        return 0;
    }
    else
        return 1;
}

/**
 * Key file parsing
 */
int g_nr_keys;
key_array_t g_key_array;

void add_keys(key_array_t ka, int kac)
{
    key_array_t new_ka = xmalloc((g_nr_keys + kac) * sizeof(struct crypto_key_t));
    memcpy(new_ka, g_key_array, g_nr_keys * sizeof(struct crypto_key_t));
    memcpy(new_ka + g_nr_keys, ka, kac * sizeof(struct crypto_key_t));
    free(g_key_array);
    g_key_array = new_ka;
    g_nr_keys += kac;
}

key_array_t read_keys(const char *key_file, int *num_keys)
{
    int size;
    FILE *fd = fopen(key_file, "r");
    if(fd == NULL)
        bugp("opening key file failed");
    fseek(fd, 0, SEEK_END);
    size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    char *buf = xmalloc(size);
    if(fread(buf, size, 1, fd) != (size_t)size)
        bugp("reading key file");
    fclose(fd);

    if(g_debug)
        printf("Parsing key file '%s'...\n", key_file);
    *num_keys = size ? 1 : 0;
    char *ptr = buf;
    /* allow trailing newline at the end (but no space after it) */
    while(ptr != buf + size && (ptr + 1) != buf + size)
    {
        if(*ptr++ == '\n')
            (*num_keys)++;
    }

    key_array_t keys = xmalloc(sizeof(struct crypto_key_t) * *num_keys);
    int pos = 0;
    for(int i = 0; i < *num_keys; i++)
    {
        /* skip ws */
        while(pos < size && isspace(buf[pos]))
            pos++;
        /* enough space ? */
        if((pos + 32) > size)
            bugp("invalid key file");
        keys[i].method = CRYPTO_KEY;
        for(int j = 0; j < 16; j++)
        {
            byte a, b;
            if(convxdigit(buf[pos + 2 * j], &a) || convxdigit(buf[pos + 2 * j + 1], &b))
                bugp(" invalid key, it should be a 128-bit key written in hexadecimal\n");
            keys[i].u.key[j] = (a << 4) | b;
        }
        if(g_debug)
        {
            printf("Add key: ");
            for(int j = 0; j < 16; j++)
                printf("%02x", keys[i].u.key[j]);
               printf("\n");
        }
        pos += 32;
    }
    free(buf);

    return keys;
}

void print_hex(byte *data, int len, bool newline)
{
    for(int i = 0; i < len; i++)
        printf("%02X ", data[i]);
    if(newline)
        printf("\n");
}

void print_key(struct crypto_key_t *key, bool newline)
{
    switch(key->method)
    {
        case CRYPTO_KEY:
            print_hex(key->u.key, 16, false);
            break;
        case CRYPTO_USBOTP:
            printf("USB-OTP(%04x:%04x)", key->u.vid_pid >> 16, key->u.vid_pid & 0xffff);
            break;
        case CRYPTO_NONE:
            printf("none");
            break;
    }
    if(newline)
        printf("\n");
}

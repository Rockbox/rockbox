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

void *memdup(const void *p, size_t len)
{
    void *cpy = xmalloc(len);
    memcpy(cpy, p, len);
    return cpy;
}

void generate_random_data(void *buf, size_t sz)
{
    size_t i = 0;
    unsigned char* p = (unsigned char*)buf;
    while(i++ < sz)
        *p++ = rand();
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

/* helper function to augment an array, free old array */
void *augment_array(void *arr, size_t elem_sz, size_t cnt, void *aug, size_t aug_cnt)
{
    void *p = xmalloc(elem_sz * (cnt + aug_cnt));
    memcpy(p, arr, elem_sz * cnt);
    memcpy(p + elem_sz * cnt, aug, elem_sz * aug_cnt);
    free(arr);
    return p;
}

/**
 * Key file parsing
 */
int g_nr_keys;
key_array_t g_key_array;

bool parse_key(char **pstr, struct crypto_key_t *key)
{
    char *str = *pstr;
    /* ignore spaces */
    while(isspace(*str))
        str++;
    /* CRYPTO_KEY: 32 hex characters
     * CRYPTO_USBOTP: usbotp(vid:pid) where vid and pid are hex numbers */
    if(isxdigit(str[0]))
    {
        if(strlen(str) < 32)
            return false;
        for(int j = 0; j < 16; j++)
        {
            byte a, b;
            if(convxdigit(str[2 * j], &a) || convxdigit(str[2 * j + 1], &b))
                return false;
            key->u.key[j] = (a << 4) | b;
        }
        /* skip key */
        *pstr = str + 32;
        key->method = CRYPTO_KEY;
        return true;
    }
    else
    {
        const char *prefix = "usbotp(";
        if(strlen(str) < strlen(prefix))
            return false;
        if(strncmp(str, prefix, strlen(prefix)) != 0)
            return false;
        str += strlen(prefix);
        /* vid */
        long vid = strtol(str, &str, 16);
        if(vid < 0 || vid > 0xffff)
            return false;
        if(*str++ != ':')
            return false;
        /* pid */
        long pid = strtol(str, &str, 16);
        if(pid < 0 || pid > 0xffff)
            return false;
        if(*str++ != ')')
            return false;
        *pstr = str;
        key->method = CRYPTO_USBOTP;
        key->u.vid_pid = vid << 16 | pid;
        return true;
    }
}

void add_keys(key_array_t ka, int kac)
{
    key_array_t new_ka = xmalloc((g_nr_keys + kac) * sizeof(struct crypto_key_t));
    memcpy(new_ka, g_key_array, g_nr_keys * sizeof(struct crypto_key_t));
    memcpy(new_ka + g_nr_keys, ka, kac * sizeof(struct crypto_key_t));
    free(g_key_array);
    g_key_array = new_ka;
    g_nr_keys += kac;
}

void clear_keys()
{
    free(g_key_array);
    g_nr_keys = 0;
    g_key_array = NULL;
}

bool add_keys_from_file(const char *key_file)
{
    int size;
    FILE *fd = fopen(key_file, "r");
    if(fd == NULL)
    {
        if(g_debug)
            perror("cannot open key file");
        return false;
    }
    fseek(fd, 0, SEEK_END);
    size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    char *buf = xmalloc(size + 1);
    if(fread(buf, 1, size, fd) != (size_t)size)
    {
        if(g_debug)
            perror("Cannot read key file");
        fclose(fd);
        return false;
    }
    buf[size] = 0;
    fclose(fd);

    if(g_debug)
        printf("Parsing key file '%s'...\n", key_file);
    char *p = buf;
    while(1)
    {
        struct crypto_key_t k;
        /* parse key */
        if(!parse_key(&p, &k))
        {
            if(g_debug)
                printf("invalid key file\n");
            return false;
        }
        if(g_debug)
        {
            printf("Add key: ");
            print_key(&k, true);
        }
        add_keys(&k, 1);
        /* request at least one space character before next key, or end of file */
        if(*p != 0 && !isspace(*p))
        {
            if(g_debug)
                printf("invalid key file\n");
            return false;
        }
        /* skip whitespace */
        while(isspace(*p))
            p++;
        if(*p == 0)
            break;
    }
    free(buf);
    return true;
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

char OFF[] = { 0x1b, 0x5b, 0x31, 0x3b, '0', '0', 0x6d, '\0' };

char GREY[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '0', 0x6d, '\0' };
char RED[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '1', 0x6d, '\0' };
char GREEN[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '2', 0x6d, '\0' };
char YELLOW[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '3', 0x6d, '\0' };
char BLUE[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '4', 0x6d, '\0' };

static bool g_color_enable = true;

void enable_color(bool enable)
{
    g_color_enable = enable;
}

void color(color_t c)
{
    if(g_color_enable)
        printf("%s", (char *)c);
}

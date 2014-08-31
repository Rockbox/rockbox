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
#include <stdarg.h>
#include "misc.h"

bool g_debug = false;
bool g_force = false;

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
    if(!r)
    {
        printf("Alloc failed\n");
        abort();
    }
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

void augment_array_ex(void **arr, size_t elem_sz, int *cnt, int *capacity,
    void *aug, int aug_cnt)
{
    /* if capacity is not large enough, double it */
    if(*cnt + aug_cnt > *capacity)
    {
        if(*capacity == 0)
            *capacity = 1;
        while(*cnt + aug_cnt > *capacity)
            *capacity *= 2;
        void *p = xmalloc(elem_sz * (*capacity));
        memcpy(p, *arr, elem_sz * (*cnt));
        free(*arr);
        *arr = p;
    }
    /* copy elements */
    memcpy(*arr + elem_sz * (*cnt), aug, elem_sz * aug_cnt);
    *cnt += aug_cnt;
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
     * CRYPTO_XOR_KEY: 256 hex characters */
    if(isxdigit(str[0]) && strlen(str) >= 256 && isxdigit(str[32]))
    {
        for(int j = 0; j < 128; j++)
        {
            byte a, b;
            if(convxdigit(str[2 * j], &a) || convxdigit(str[2 * j + 1], &b))
                return false;
            key->u.xor_key[j / 64].key[j % 64] = (a << 4) | b;
        }
        /* skip key */
        *pstr = str + 256;
        key->method = CRYPTO_XOR_KEY;
        return true;
    }
    else if(isxdigit(str[0]))
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
        return false;
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

void misc_std_printf(void *user, const char *fmt, ...)
{
    (void) user;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
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
            print_key(NULL, misc_std_printf, &k, true);
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

void print_hex(void *user, misc_printf_t printf, byte *data, int len, bool newline)
{
    for(int i = 0; i < len; i++)
        printf(user, "%02X ", data[i]);
    if(newline)
        printf(user, "\n");
}

void print_key(void *user, misc_printf_t printf, struct crypto_key_t *key, bool newline)
{
    switch(key->method)
    {
        case CRYPTO_KEY:
            print_hex(user, printf, key->u.key, 16, false);
            break;
        case CRYPTO_NONE:
            printf(user, "none");
            break;
        case CRYPTO_XOR_KEY:
            print_hex(user, printf, &key->u.xor_key[0].key[0], 64, false);
            print_hex(user, printf, &key->u.xor_key[1].key[0], 64, false);
            break;
        default:
            printf(user, "unknown");
    }
    if(newline)
        printf(user, "\n");
}

const char OFF[] = { 0x1b, 0x5b, 0x31, 0x3b, '0', '0', 0x6d, '\0' };

const char GREY[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '0', 0x6d, '\0' };
const char RED[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '1', 0x6d, '\0' };
const char GREEN[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '2', 0x6d, '\0' };
const char YELLOW[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '3', 0x6d, '\0' };
const char BLUE[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '4', 0x6d, '\0' };

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

void generic_std_printf(void *u, bool err, color_t c, const char *f, ...)
{
    (void)u;
    if(!g_debug && !err)
        return;
    va_list args;
    va_start(args, f);
    color(c);
    vprintf(f, args);
    va_end(args);
}

enum sb_version_guess_t guess_sb_version(const char *filename)
{
#define ret(x) do { if(f) fclose(f); return x; } while(0)
    FILE *f = fopen(filename, "rb");
    if(f == NULL)
        ret(SB_VERSION_ERR);
    // check signature
    uint8_t sig[4];
    if(fseek(f, 20, SEEK_SET))
        ret(SB_VERSION_UNK);
    if(fread(sig, 4, 1, f) != 1)
        ret(SB_VERSION_UNK);
    if(memcmp(sig, "STMP", 4) != 0)
        ret(SB_VERSION_UNK);
    // check header size (v1)
    uint32_t hdr_size;
    if(fseek(f, 8, SEEK_SET))
        ret(SB_VERSION_UNK);
    if(fread(&hdr_size, 4, 1, f) != 1)
        ret(SB_VERSION_UNK);
    if(hdr_size == 0x34)
        ret(SB_VERSION_1);
    // check header params relationship
    struct
    {
        uint16_t nr_keys; /* Number of encryption keys */
        uint16_t key_dict_off; /* Offset to key dictionary (in blocks) */
        uint16_t header_size; /* In blocks */
        uint16_t nr_sections; /* Number of sections */
        uint16_t sec_hdr_size; /* Section header size (in blocks) */
    } __attribute__((packed)) u;
    if(fseek(f, 0x28, SEEK_SET))
        ret(SB_VERSION_UNK);
    if(fread(&u, sizeof(u), 1, f) != 1)
        ret(SB_VERSION_UNK);
    if(u.sec_hdr_size == 1 && u.header_size == 6 && u.key_dict_off == u.header_size + u.nr_sections)
        ret(SB_VERSION_2);
    ret(SB_VERSION_UNK);
#undef ret
}

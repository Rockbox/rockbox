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
#ifndef __MISC_H__
#define __MISC_H__

#include <stdbool.h>
#include "crypto.h"

#define _STR(a) #a
#define STR(a) _STR(a)

#define bug(...) do { fprintf(stderr,"["__FILE__":"STR(__LINE__)"]ERROR: "__VA_ARGS__); exit(1); } while(0)
#define bugp(...) do { fprintf(stderr, __VA_ARGS__); perror(" "); exit(1); } while(0)

#define ROUND_UP(val, round) ((((val) + (round) - 1) / (round)) * (round))

extern bool g_debug;

typedef struct crypto_key_t *key_array_t;
int g_nr_keys;
key_array_t g_key_array;

void *memdup(const void *p, size_t len);
void *augment_array(void *arr, size_t elem_sz, size_t cnt, void *aug, size_t aug_cnt);
void generate_random_data(void *buf, size_t sz);
void *xmalloc(size_t s);
int convxdigit(char digit, byte *val);
void print_hex(byte *data, int len, bool newline);
void add_keys(key_array_t ka, int kac);
bool parse_key(char **str, struct crypto_key_t *key);
bool add_keys_from_file(const char *key_file);
void print_key(struct crypto_key_t *key, bool newline);
void clear_keys();

typedef char color_t[];

extern color_t OFF, GREY, RED, GREEN, YELLOW, BLUE;
void color(color_t c);
void enable_color(bool enable);

#endif /* __MISC_H__ */

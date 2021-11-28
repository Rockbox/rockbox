/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "xf_flashmap.h"
#include "xf_error.h"
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static int xdigit_to_int(char c)
{
    if(c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if(c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    if(c >= '0' && c <= '9')
        return c - '0';
    return -1;
}

static int isfilenamechar(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '$' || c == '%' || c == '\'' || c == '-' || c == '_' ||
           c == '@' || c == '~' || c == '`' || c == '!' || c == '(' ||
           c == ')' || c == '{' || c == '}' || c == '^' || c == '#' ||
           c == '&' || c == '+' || c == ',' || c == ';' || c == '=' ||
           c == '[' || c == ']' || c == '.';
}

int xf_map_parseline(const char* line, struct xf_map* map)
{
    enum {
        s_name,
        s_md5,
        s_first_num,
        s_file_len = s_first_num,
        s_offset,
        s_length,
        s_done,
    };

#define skipws() do { while(*line == ' ' || *line == '\t') ++line; } while(0)
#define nextstate() do { ++state; length = 0; ++line; skipws(); } while(0)

    int state = s_name;
    int length = 0;
    int digit_val;
    uint32_t int_val;
    uint32_t* num_ptr[3] = {&map->file_length, &map->offset, &map->length};
    bool has_md5 = true;

    skipws();
    while(*line && *line != '\n') {
        switch(state) {
        case s_name:
            if(*line == ' ' || *line == '\t') {
                nextstate();
                continue;
            } else if(isfilenamechar(*line)) {
                if(length == XF_MAP_NAMELEN)
                    return XF_E_FILENAME_TOO_LONG;

                map->name[length++] = *line++;
                map->name[length] = '\0';
                continue;
            } else {
                return XF_E_SYNTAX_ERROR;
            }

        case s_md5:
            if(*line == '-') {
                memset(map->md5, 0, 16);
                map->file_length = 0;
                has_md5 = false;
                ++line;
            } else {
                for(int i = 0; i < 16; ++i) {
                    int_val = 0;
                    for(int j = 0; j < 2; ++j) {
                        digit_val = xdigit_to_int(*line++);
                        if(digit_val < 0)
                            return XF_E_SYNTAX_ERROR;

                        int_val <<= 4;
                        int_val |= digit_val;
                    }

                    map->md5[i] = int_val;
                }
            }

            if(*line == ' ' || *line == '\t') {
                /* skip file length if md5 is not present */
                if(!has_md5)
                    ++state;

                nextstate();
                continue;
            } else {
                return XF_E_SYNTAX_ERROR;
            }

        case s_file_len:
        case s_offset:
        case s_length:
            int_val = 0;

            if(*line == '0') {
                ++line;
                if(*line == 'x' || *line == 'X') {
                    ++line;
                    while((digit_val = xdigit_to_int(*line)) >= 0) {
                        ++line;

                        if(int_val > UINT32_MAX/16)
                            return XF_E_INT_OVERFLOW;

                        int_val *= 16;
                        int_val |= digit_val;
                    }
                }
            } else if(*line >= '1' && *line <= '9') {
                do {
                    if(int_val > UINT32_MAX/10)
                        return XF_E_INT_OVERFLOW;
                    int_val *= 10;

                    digit_val = *line++ - '0';
                    if(int_val > UINT32_MAX - digit_val)
                        return XF_E_INT_OVERFLOW;

                    int_val += digit_val;
                } while(*line >= '0' && *line <= '9');
            }

            *num_ptr[state - s_first_num] = int_val;

            if(*line == ' ' || *line == '\t') {
                nextstate();
                continue;
            } else if(state+1 == s_done && *line == '\0') {
                /* end of input */
                continue;
            } else {
                return XF_E_SYNTAX_ERROR;
            }

        case s_done:
            if(isspace(*line)) {
                line++;
                continue; /* swallow trailing spaces, carriage return, etc */
            } else
                return XF_E_SYNTAX_ERROR;
        }
    }

#undef skipws
#undef nextstate

    /* one last overflow check - ensure mapped range is addressable */
    if(map->offset > UINT32_MAX - map->length)
        return XF_E_INT_OVERFLOW;

    if(has_md5)
        map->flags = XF_MAP_HAS_MD5 | XF_MAP_HAS_FILE_LENGTH;
    else
        map->flags = 0;

    return XF_E_SUCCESS;
}

struct map_parse_args {
    struct xf_map* map;
    int num;
    int maxnum;
};

int map_parse_line_cb(int n, char* buf, void* arg)
{
    (void)n;

    struct map_parse_args* args = arg;

    /* skip whitespace */
    while(*buf && isspace(*buf))
        ++buf;

    /* ignore comments and blank lines */
    if(*buf == '#' || *buf == '\0')
        return 0;

    struct xf_map dummy_map;
    struct xf_map* dst_map;
    if(args->num < args->maxnum)
        dst_map = &args->map[args->num];
    else
        dst_map = &dummy_map;

    int rc = xf_map_parseline(buf, dst_map);
    if(rc)
        return rc;

    args->num++;
    return 0;
}

int xf_map_parse(struct xf_stream* s, struct xf_map* map, int maxnum)
{
    char buf[200];
    struct map_parse_args args;
    args.map = map;
    args.num = 0;
    args.maxnum = maxnum;

    int rc = xf_stream_read_lines(s, buf, sizeof(buf),
                                  map_parse_line_cb, &args);
    if(rc < 0)
        return rc;

    return args.num;
}

static int xf_map_compare(const void* a, const void* b)
{
    const struct xf_map* mapA = a;
    const struct xf_map* mapB = b;

    if(mapA->offset < mapB->offset)
        return -1;
    else if(mapA->offset == mapB->offset)
        return 0;
    else
        return 1;
}

int xf_map_sort(struct xf_map* map, int num)
{
    qsort(map, num, sizeof(struct xf_map), xf_map_compare);
    return xf_map_validate(map, num);
}

int xf_map_validate(const struct xf_map* map, int num)
{
    for(int i = 1; i < num; ++i)
        if(map[i].offset <= map[i-1].offset)
            return -1;

    for(int i = 1; i < num; ++i)
        if(map[i-1].offset + map[i-1].length > map[i].offset)
            return i;

    return 0;
}

int xf_map_write(struct xf_map* map, int num, struct xf_stream* s)
{
    static const char hex[] = "0123456789abcdef";
    char buf[200];
    char md5str[33];
    int total_len = 0;

    md5str[32] = '\0';

    for(int i = 0; i < num; ++i) {
        bool has_md5 = false;
        if(map->flags & XF_MAP_HAS_MD5) {
            if(!(map->flags & XF_MAP_HAS_FILE_LENGTH))
                return XF_E_INVALID_PARAMETER;

            has_md5 = true;
            for(int j = 0; j < 16; ++j) {
                uint8_t byte = map[i].md5[j];
                md5str[2*j]   = hex[(byte >> 4) & 0xf];
                md5str[2*j+1] = hex[byte & 0xf];
            }
        }

        int len;
        if(!has_md5) {
            len = snprintf(buf, sizeof(buf), "%s - %lx %lu\n",
                           map[i].name,
                           (unsigned long)map[i].offset,
                           (unsigned long)map[i].length);
        } else {
            len = snprintf(buf, sizeof(buf), "%s %s %lu 0x%lx %lu\n",
                           map[i].name, md5str,
                           (unsigned long)map[i].file_length,
                           (unsigned long)map[i].offset,
                           (unsigned long)map[i].length);
        }

        if(len < 0 || (size_t)len >= sizeof(buf))
            return XF_E_LINE_TOO_LONG;

        if(s) {
            int rc = xf_stream_write(s, buf, len);
            if(rc != len)
                return XF_E_IO;
        }

        total_len += len;
    }

    return total_len;
}

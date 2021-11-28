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

#include "test.h"
#include "xf_stream.h"
#include "xf_flashmap.h"
#include "xf_error.h"
#include "file.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define MD5SUM \
    "abcdef012345ABCDEF012345abcdef01"
#define MD5SUM_L \
    "abcdef012345abcdef012345abcdef01"
#define LONG_NAME \
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
#define OVERLONG_NAME \
    LONG_NAME "a"

static const struct parseline_test {
    const char* line;
    int rc;
    const char* name;
    const char* md5;
    uint32_t file_length;
    uint32_t offset;
    uint32_t length;
    int flags;
} parseline_tests[] = {
    {"test1 " MD5SUM " 1234 0x9876 42",
     XF_E_SUCCESS, "test1", MD5SUM_L, 1234, 0x9876, 42,
     XF_MAP_HAS_FILE_LENGTH|XF_MAP_HAS_MD5},
    {"    test1 " MD5SUM " 1234 0x9876 42",
     XF_E_SUCCESS, "test1", MD5SUM_L, 1234, 0x9876, 42,
     XF_MAP_HAS_FILE_LENGTH|XF_MAP_HAS_MD5},
    {"test1 " MD5SUM " 1234 0x9876 42     \r",
     XF_E_SUCCESS, "test1", MD5SUM_L, 1234, 0x9876, 42,
     XF_MAP_HAS_FILE_LENGTH|XF_MAP_HAS_MD5},
    {"test1 " MD5SUM " 1234 0x9876 42 # junk",
     XF_E_SYNTAX_ERROR, NULL, NULL, 0, 0, 0, 0},
    {OVERLONG_NAME,
     XF_E_FILENAME_TOO_LONG, NULL, NULL, 0, 0, 0, 0},
    {LONG_NAME " " MD5SUM " 1234 0x9876 42",
     XF_E_SUCCESS, LONG_NAME, MD5SUM_L, 1234, 0x9876, 42,
     XF_MAP_HAS_FILE_LENGTH|XF_MAP_HAS_MD5},
    {"test1 " MD5SUM " 4294967295 0 0xffffffff",
     XF_E_SUCCESS, "test1", MD5SUM_L, 4294967295, 0, 0xfffffffful,
     XF_MAP_HAS_FILE_LENGTH|XF_MAP_HAS_MD5},
    {"test1 " MD5SUM " 4294967295 1 0xffffffff",
     XF_E_INT_OVERFLOW, NULL, NULL, 0, 0, 0, 0},
    {"test1 " MD5SUM " 4294967296 0 0xffffffff",
     XF_E_INT_OVERFLOW, NULL, NULL, 0, 0, 0, 0},
    {"test1 " MD5SUM " 100294967295 0 0xffffffff",
     XF_E_INT_OVERFLOW, NULL, NULL, 0, 0, 0, 0},
    {"test1 " MD5SUM " 0 0 0xffffffff0",
     XF_E_INT_OVERFLOW, NULL, NULL, 0, 0, 0, 0},
    {"test1 " MD5SUM " 0 0 0x100000000",
     XF_E_INT_OVERFLOW, NULL, NULL, 0, 0, 0, 0},
    {"test1 " MD5SUM " 0 0 0x100000001",
     XF_E_INT_OVERFLOW, NULL, NULL, 0, 0, 0, 0},
    {"<",
     XF_E_SYNTAX_ERROR, NULL, NULL, 0, 0, 0, 0},
    {"test1 abcXdef",
     XF_E_SYNTAX_ERROR, NULL, NULL, 0, 0, 0, 0},
    {"test1 " MD5SUM "0",
     XF_E_SYNTAX_ERROR, NULL, NULL, 0, 0, 0, 0},
    {"test1 - 4567 0xabcd",
     XF_E_SUCCESS, "test1", NULL, 0, 4567, 0xabcd, 0},
    {"test1 - 0 0x123456789",
     XF_E_INT_OVERFLOW, NULL, NULL, 0, 0, 0, 0},
    {"test1 - 4f",
     XF_E_SYNTAX_ERROR, NULL, NULL, 0, 0, 0, 0},
};

static void md5_to_string(char* buf, const uint8_t* md5)
{
    const char* hex = "0123456789abcdef";
    for(int i = 0; i < 16; ++i) {
        uint8_t byte = md5[i];
        buf[2*i]   = hex[(byte >> 4) & 0xf];
        buf[2*i+1] = hex[byte & 0xf];
    }

    buf[32] = '\0';
}

int test_flashmap_parseline(void)
{
    int rc;
    struct xf_map map;
    char md5string[33];

    size_t num_cases = sizeof(parseline_tests) / sizeof(struct parseline_test);
    const struct parseline_test* test = &parseline_tests[0];

    for(; num_cases > 0; --num_cases, ++test) {
        rc = xf_map_parseline(test->line, &map);

        T_ASSERT(rc == test->rc);
        if(rc != XF_E_SUCCESS)
            continue;

        T_ASSERT(strcmp(map.name, test->name) == 0);
        T_ASSERT(map.offset == test->offset);
        T_ASSERT(map.length == test->length);
        T_ASSERT(map.flags == test->flags);

        if(test->flags & XF_MAP_HAS_MD5) {
            md5_to_string(md5string, map.md5);
            T_ASSERT(strcmp(md5string, test->md5) == 0);
        }

        if(test->flags & XF_MAP_HAS_FILE_LENGTH) {
            T_ASSERT(map.file_length == test->file_length);
        }
    }

  cleanup:
    return rc;
}

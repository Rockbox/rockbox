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
#include "xf_error.h"
#include "file.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define TESTDATADIR         "test/data/"
#define TESTFILE_SHUFFLED   TESTDATADIR "lines_shuffled.txt"
#define TESTFILE_SORTED     TESTDATADIR "lines_sorted.txt"

const char line_string[] = "1234567890abcdefghijklmnopqrstuvwxyz";

static int read_line_cb(int n, char* buf, void* arg)
{
    int* max_n = (int*)arg;
    *max_n = n;

    T_ASSERT(strlen(buf) > 0);
    T_ASSERT(strncmp(line_string, buf, strlen(buf)) == 0);

  cleanup:
    return 0;
}

void test_stream_read_lines(void)
{
    struct xf_stream s;
    int rc;
    char buf[100];
    bool s_open = false;

    for(int bufsz = 38; bufsz <= 100; bufsz += 1) {
        rc = xf_open_file(TESTFILE_SHUFFLED, O_RDONLY, &s);
        T_ASSERT(rc == 0);
        s_open = true;

        int max_n = 0;
        rc = xf_stream_read_lines(&s, buf, bufsz, &read_line_cb, &max_n);
        T_ASSERT(rc == 0);
        T_ASSERT(max_n+1 == 108);

        xf_stream_close(&s);
        s_open = false;
    }

  cleanup:
    if(s_open)
        xf_stream_close(&s);
    return;
}

void test_stream_read_line_too_long(void)
{
    struct xf_stream s;
    int rc;
    char buf[38];
    bool s_open = false;

    for(int bufsz = 0; bufsz <= 38; bufsz += 1) {
        rc = xf_open_file(TESTFILE_SORTED, O_RDONLY, &s);
        T_ASSERT(rc == 0);
        s_open = true;

        int max_n = -1;
        rc = xf_stream_read_lines(&s, buf, bufsz, &read_line_cb, &max_n);
        if(bufsz == 38) {
            T_ASSERT(rc == 0);
            T_ASSERT(max_n+1 == 36);
        } else {
            T_ASSERT(rc == XF_E_LINE_TOO_LONG);
            if(bufsz <= 1)
                T_ASSERT(max_n == -1);
            else
                T_ASSERT(max_n+1 == bufsz-2);
        }

        xf_stream_close(&s);
        s_open = false;
    }

  cleanup:
    if(s_open)
        xf_stream_close(&s);
    return;
}

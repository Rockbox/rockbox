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

#include <stdio.h>

static int test_num_executed = 0;
static int test_num_failed = 0;
int test_num_asserts_executed = 0;
int test_num_asserts_failed = 0;

void test_failure(const char* file, int line, const char* msg)
{
    fprintf(stderr, "%s:%d: ASSERTION FAILED: %s\n", file, line, msg);
    ++test_num_asserts_failed;
}

typedef void(*test_t)(void);

struct test_info {
    const char* name;
    test_t func;
};

extern void test_stream_read_lines(void);
extern void test_stream_read_line_too_long(void);
extern void test_flashmap_parseline(void);

#define TEST(x) {#x, x}
static const struct test_info all_tests[] = {
    TEST(test_stream_read_lines),
    TEST(test_stream_read_line_too_long),
    TEST(test_flashmap_parseline),
};
#undef TEST

void run_test(const struct test_info* tinfo)
{
    int asserts_now = test_num_asserts_failed;
    ++test_num_executed;

    fprintf(stderr, "RUN %s\n", tinfo->name);
    tinfo->func();

    if(test_num_asserts_failed > asserts_now) {
        fprintf(stderr, "  %s: FAILED!\n", tinfo->name);
        ++test_num_failed;
    }
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    size_t num_tests = sizeof(all_tests) / sizeof(struct test_info);
    for(size_t i = 0; i < num_tests; ++i)
        run_test(&all_tests[i]);

    fprintf(stderr, "------------------------------------------\n");
    fprintf(stderr, "TEST COMPLETE\n");
    fprintf(stderr, "  Tests            %d failed / %d executed\n",
            test_num_failed, test_num_executed);
    fprintf(stderr, "  Assertions       %d failed / %d executed\n",
            test_num_asserts_failed, test_num_asserts_executed);

    if(test_num_failed > 0)
        return 1;
}

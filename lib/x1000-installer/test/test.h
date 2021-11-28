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

#ifndef TEST_H
#define TEST_H

extern int test_num_asserts_executed;
extern int test_num_asserts_failed;

extern void test_failure(const char* file, int line, const char* msg);

#define T_ASSERT(cond)                               \
    do {                                             \
        ++test_num_asserts_executed;                 \
        if(!(cond)) {                                \
            test_failure(__FILE__, __LINE__, #cond); \
            goto cleanup;                            \
        }                                            \
    } while(0)

#endif

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

char OFF[] = { 0x1b, 0x5b, 0x31, 0x3b, '0', '0', 0x6d, '\0' };

char GREY[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '0', 0x6d, '\0' };
char RED[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '1', 0x6d, '\0' };
char GREEN[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '2', 0x6d, '\0' };
char YELLOW[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '3', 0x6d, '\0' };
char BLUE[] = { 0x1b, 0x5b, 0x31, 0x3b, '3', '4', 0x6d, '\0' };

static bool g_color_enable = true;

void *xmalloc(size_t s)
{
    void * r = malloc(s);
    if(!r) bugp("malloc");
    return r;
}

void enable_color(bool enable)
{
    g_color_enable = enable;
}

void color(color_t c)
{
    if(g_color_enable)
        printf("%s", (char *)c);
}

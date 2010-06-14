/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: tag_table.c 26346 2010-05-28 02:30:27Z jdgordon $
 *
 * Copyright (C) 2010 Jonathan Gordon
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
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "skin_parser.h"
#include "tag_table.h"
#ifndef SKIN_STRUCTS_H_
#define SKIN_STRUCTS_H_
struct skin 
{
};


struct progressbar {
    enum skin_token_type type;
    struct viewport *vp;
    /* regular pb */
    short x;
    /* >=0: explicitly set in the tag -> y-coord within the viewport
       <0 : not set in the tag -> negated 1-based line number within
            the viewport. y-coord will be computed based on the font height */
    short y;
    short width;
    short height;
    bool  follow_lang_direction;
    /*progressbar image*/
 //   struct bitmap bm;
    bool have_bitmap_pb;
};

struct line {
    unsigned update_mode;
    bool eat_line_ending;
};

struct subline {
    int timeout;
    int current_line;
    unsigned long last_change_tick;
};

struct conditional {
    int last_value;
};


#endif

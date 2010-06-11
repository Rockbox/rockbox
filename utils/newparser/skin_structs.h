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

struct skin_token {
    enum skin_token_type type; /* enough to store the token type */

    /* Whether the tag (e.g. track name or the album) refers the
       current or the next song (false=current, true=next) */
    bool next;

    union {
        char c;
        unsigned int i;
        void* data;
    } value;
};

#define MAX_TOKENS 10000
struct skin 
{
    int token_count;
    struct skin_token tokens[MAX_TOKENS];
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

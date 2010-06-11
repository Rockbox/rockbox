/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: skin_parser.c 26752 2010-06-10 21:22:16Z bieber $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "skin_parser.h"
#include "skin_debug.h"
#include "tag_table.h"
#include "symbols.h"
#include "skin_scan.h"

void skin_render_alternator(struct skin_element* alternator);

/* Draw a LINE element onto the display */
void skin_render_line(struct skin_element* line)
{
    int i=0, value;
    struct skin_element *child = line->children[0];
    while (child)
    {
        switch (child->type)
        {
            case CONDITIONAL:
                value = 0; /* actually get it from the token :p */
                if (value >= child->children_count)
                    value = child->children_count-1;
                if (child->children[value]->type == SUBLINES)
                    skin_render_alternator(child->children[value]);
                else if (child->children[value]->type == LINE)
                    skin_render_line(child->children[value]);
                break;
            case TAG:
                printf("%%%s", child->tag->name);
                break;
            case TEXT:
                printf("%s", (char*)(child->data));
                break;
            case COMMENT:
            default:
                break;
        }
        child = child->next;
    }
    printf("\n"); /* might be incorrect */
}

void skin_render_alternator(struct skin_element* alternator)
{
    /*TODO Choose which subline to draw */
    skin_render_line(alternator->children[0]);
}

void skin_render_viewport(struct skin_element* viewport)
{
    struct skin_element *line = viewport;
    while (line)
    {
        if (line->type == SUBLINES)
            skin_render_alternator(line);
        else if (line->type == LINE)
            skin_render_line(line);
        line = line->next;
    }
}

void skin_render(struct skin_element* root)
{
    struct skin_element* viewport = root;
    while (viewport)
    {
        skin_render_viewport(viewport->children[viewport->children_count-1]);
        viewport = viewport->next;
    }
}

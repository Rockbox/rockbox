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
#include <stdbool.h>
#include <ctype.h>

#include "skin_parser.h"
#include "skin_debug.h"
#include "tag_table.h"
#include "symbols.h"
#include "skin_scan.h"
#include "skin_structs.h"

#define MAX_LINE 1024

typedef void (*skin_render_func)(struct skin_element* alternator, 
                            char* buf, size_t buf_size, int line_number);
void skin_render_alternator(struct skin_element* alternator, 
                            char* buf, size_t buf_size, int line_number);

static void do_tags_in_hidden_conditional(struct skin_element* branch)
{
    /* Tags here are ones which need to be "turned off" or cleared 
     * if they are in a conditional branch which isnt being used */
    if (branch->type == SUBLINES)
    {
        int i;
        for (i=0; i<branch->children_count; i++)
        {
            do_tags_in_hidden_conditional(branch->children[i]);
        }
    }
    else if (branch->type == LINE)
    {
        struct skin_element *child = branch->children[0];
        while (child)
        {
            if (child->type != TAG)
            {
                child = child->next;
                continue;
            }
            switch (child->tag->type)
            {
                case SKIN_TOKEN_PEAKMETER:
                    /* stop the peak meter */
                    break;
                case SKIN_TOKEN_ALBUMART_DISPLAY:
                    /* clear the AA image */
                    break;
                case SKIN_TOKEN_IMAGE_DISPLAY:
                case SKIN_TOKEN_IMAGE_PRELOAD_DISPLAY:
                    printf("disable image\n");
                    /* clear images */
                    break;
                default:
                    break;
            }
            child = child->next;
        }
    }
}
        
    
/* Draw a LINE element onto the display */
void skin_render_line(struct skin_element* line, 
                      char* buf, size_t buf_size, int line_number)
{
    int last_value, value;
    if (line->children_count == 0)
        return; /* empty line, do nothing */
    struct skin_element *child = line->children[0];
    skin_render_func func = skin_render_line;
    char tempbuf[128];
    while (child)
    {
        tempbuf[0] = '\0';
        switch (child->type)
        {
            case CONDITIONAL:
                last_value = ((struct conditional*)(child->data))->last_value;
                value = 0; /* actually get it from the token :p */
                if (value >= child->children_count)
                    value = child->children_count-1;
                    
                /* some tags need handling if they are being disabled */
                if (value != last_value && last_value < child->children_count)
                    do_tags_in_hidden_conditional(child->children[last_value]);
                last_value = value;
                
                if (child->children[value]->type == SUBLINES)
                    func = skin_render_alternator;
                else if (child->children[value]->type == LINE)
                    func = skin_render_line;
                func(child->children[value], buf, buf_size, line_number);
                break;
            case TAG:
                snprintf(tempbuf, sizeof(tempbuf), "%%%s", child->tag->name);
                break;
            case TEXT:
                snprintf(tempbuf, sizeof(tempbuf), "%s", (char*)(child->data));
                break;
            case COMMENT:
            default:
                break;
        }
        strcat(buf, tempbuf);
        child = child->next;
    }
}
#define TIME_AFTER(a,b) 1
void skin_render_alternator(struct skin_element* alternator, 
                            char* buf, size_t buf_size, int line_number)
{
    struct subline *subline = (struct subline*)alternator->data;
    if (TIME_AFTER(subline->last_change_tick + subline->timeout, 0/*FIXME*/))
    {
        subline->current_line++;
        if (subline->current_line >= alternator->children_count)
            subline->current_line = 0;
    }
    skin_render_line(alternator->children[subline->current_line],
                     buf, buf_size, line_number);
}

void skin_render_viewport(struct skin_element* viewport, bool draw_tags)
{
    int line_number = 0;
    char linebuf[MAX_LINE];
    skin_render_func func = skin_render_line;
    struct skin_element* line = viewport;
    while (line)
    {
        linebuf[0] = '\0';
        if (line->type == SUBLINES)
            func = skin_render_alternator;
        else if (line->type == LINE)
            func = skin_render_line;
        
        func(line, linebuf, sizeof(linebuf), line_number);
        if (draw_tags)
        {
            printf("[%d]%s", line_number, linebuf);
            if (!((struct line*)line->data)->eat_line_ending)
            {
                printf("\n");
            }
        }
        if (!((struct line*)line->data)->eat_line_ending)
            line_number++;
        line = line->next;
    }
}

void skin_render(struct skin_element* root)
{
    struct skin_element* viewport = root;
    bool draw_tags = viewport->next ? false : true;
    while (viewport)
    {
        skin_render_viewport(viewport->children[0], draw_tags);
        draw_tags = true;
        viewport = viewport->next;
    }
}

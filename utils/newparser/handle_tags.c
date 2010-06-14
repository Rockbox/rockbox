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
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "symbols.h"
#include "skin_parser.h"
#include "tag_table.h"
#include "skin_structs.h"

typedef int (tag_handler)(struct skin *skin, struct skin_element* element, bool size_only);



int handle_translate_string(struct skin *skin, struct skin_element* element, bool size_only)
{
    return 0;
}

int handle_this_or_next_track(struct skin *skin, struct skin_element* element, bool size_only)
{
    if (element->tag->type == SKIN_TOKEN_FILE_DIRECTORY)
    {
        if (element->params_count != 1 || element->params[0].type_code != NUMERIC)
            return -1;
        //token->value.i = element->params[0].data.numeric;
    }
    return 0;
}

int handle_bar(struct skin *skin, struct skin_element* element, bool size_only)
{
    struct progressbar bar;
    /* %bar with no params is different for each one so handle that! */
    if (element->params_count == 0)
    {
        if (size_only)
        {
            if (element->tag->type == SKIN_TOKEN_PROGRESSBAR)
                return sizeof(struct progressbar);
            else
                return 0;
        }
    }
    else
    {
        if (size_only)
            return sizeof(struct progressbar);
    }
    
    return 0;
}

struct tag_handler_table {
    enum skin_token_type type;
    int flags;
    tag_handler *func;
};
#define EAT_LINE_ENDING 0x01

struct tag_handler_table table[] = {
    { SKIN_TOKEN_ENABLE_THEME, EAT_LINE_ENDING, NULL },
    { SKIN_TOKEN_DISABLE_THEME, EAT_LINE_ENDING, NULL },
    /* file tags */
    { SKIN_TOKEN_FILE_BITRATE , 0, handle_this_or_next_track },
    { SKIN_TOKEN_FILE_CODEC , 0, handle_this_or_next_track },
    { SKIN_TOKEN_FILE_FREQUENCY , 0, handle_this_or_next_track },
    { SKIN_TOKEN_FILE_FREQUENCY_KHZ , 0, handle_this_or_next_track },
    { SKIN_TOKEN_FILE_NAME_WITH_EXTENSION , 0, handle_this_or_next_track },
    { SKIN_TOKEN_FILE_NAME , 0, handle_this_or_next_track },
    { SKIN_TOKEN_FILE_PATH , 0, handle_this_or_next_track },
    { SKIN_TOKEN_FILE_SIZE , 0, handle_this_or_next_track },
    { SKIN_TOKEN_FILE_VBR , 0, handle_this_or_next_track },
    { SKIN_TOKEN_FILE_DIRECTORY , 0, handle_this_or_next_track },
    /* track metadata */
    { SKIN_TOKEN_METADATA_ARTIST , 0, handle_this_or_next_track },
    { SKIN_TOKEN_METADATA_COMPOSER , 0, handle_this_or_next_track },
    { SKIN_TOKEN_METADATA_ALBUM , 0, handle_this_or_next_track },
    { SKIN_TOKEN_METADATA_ALBUM_ARTIST , 0, handle_this_or_next_track },
    { SKIN_TOKEN_METADATA_GROUPING , 0, handle_this_or_next_track },
    { SKIN_TOKEN_METADATA_GENRE , 0, handle_this_or_next_track },
    { SKIN_TOKEN_METADATA_DISC_NUMBER , 0, handle_this_or_next_track },
    { SKIN_TOKEN_METADATA_TRACK_NUMBER , 0, handle_this_or_next_track },
    { SKIN_TOKEN_METADATA_TRACK_TITLE , 0, handle_this_or_next_track },
    { SKIN_TOKEN_METADATA_VERSION , 0, handle_this_or_next_track },
    { SKIN_TOKEN_METADATA_YEAR , 0, handle_this_or_next_track },
    { SKIN_TOKEN_METADATA_COMMENT , 0, handle_this_or_next_track },
    /* misc */
    { SKIN_TOKEN_TRANSLATEDSTRING, 0, handle_translate_string},
};

int handle_tree(struct skin *skin, struct skin_element* tree, struct line *line)
{
    /* for later.. do this in two steps
     * 1) count how much skin buffer is needed
     * 2) do the actual tree->skin conversion
     */
    struct skin_element* element = tree;
    struct line *current_line = line;
    int counter;
    while (element)
    {
        if (element->type == VIEWPORT)
        {
            struct skin_element *next;
            /* parse the viewport */
            /* if the next element is a LINE we need to set it to eat the line ending */
            next = element->children[0];
            if (element->tag && next->type == LINE &&
                element->line == next->line)
            {
                struct line *newline = (struct line*)skin_alloc(sizeof(struct line));
                newline->update_mode = 0;
                newline->eat_line_ending = true;
                next->data = newline;
            }
        }
        else if (element->type == LINE && !element->data)
        {
            struct line *line = (struct line*)skin_alloc(sizeof(struct line));
            line->update_mode = 0;
            line->eat_line_ending = false;
            element->data = line;
            current_line = line;
        }
        else if (element->type == SUBLINES)
        {
            struct subline *subline = skin_alloc(sizeof(struct subline));
            subline->current_line = -1;
            subline->last_change_tick = 0;
            element->data = subline;
        }
        else if (element->type == CONDITIONAL)
        {
            struct conditional *cond = skin_alloc(sizeof(struct conditional));
            cond->last_value = element->children_count;
            element->data = cond;
        }
        else if (element->type == TAG)
        {
            int i; 
            for(i=0;i<sizeof(table)/sizeof(*table);i++)
            {
                if (table[i].type == element->tag->type)
                {
                    if (table[i].func)
                        table[i].func(skin, element, false);
                    if (table[i].flags&EAT_LINE_ENDING)
                        line->eat_line_ending = true;
                    break;
                }
            }
        }
        else if (element->type == TEXT)
        {
            /* handle */
        }
        
        counter = 0;
        while (counter < element->children_count)
        {
            int ret = handle_tree(skin, element->children[counter], current_line);
            counter++;
        }
        /* *probably* set current_line to NULL here */
        element = element->next;
    }
    return 0;
}

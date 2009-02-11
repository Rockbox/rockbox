/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007-2009 Joshua Simmons <mud at majidejima dot com>
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

#include "goban.h"
#include "sgf_output.h"
#include "sgf.h"
#include "sgf_storage.h"
#include "util.h"
#include "board.h"
#include "game.h"

static void pos_to_sgf (unsigned short pos, char *buffer);

static void output_prop (int prop_handle);
static void output_all_props (void);
static bool output_current_node (void);
static void output_gametree (void);
static void output_header_props (void);
static bool output_header_helper (enum prop_type_t type);
static int stupid_num_variations (void);

bool
output_sgf (const char *filename)
{
    int current = -1;
    union prop_data_t temp_data;
    int saved = current_node;

    sgf_fd = create_or_open_file (filename);

    if (sgf_fd < 0)
    {
        return false;
    }

    DEBUGF ("outputting to: %s (%d)\n", filename, sgf_fd);

    empty_stack (&parse_stack);

    rb->lseek (sgf_fd, 0, SEEK_SET);
    rb->ftruncate (sgf_fd, 0);

    if (sgf_fd < 0)
    {
        return false;
    }

    if (tree_head < 0)
    {
        close_file (&sgf_fd);
        return false;
    }

    push_int_stack (&parse_stack, tree_head);

    while (pop_int_stack (&parse_stack, &current))
    {
        int var_to_process = 0;
        int temp_prop =
            get_prop_sgf (current, PROP_VARIATION_TO_PROCESS, NULL);

        if (temp_prop >= 0)
        {
            var_to_process = get_prop (temp_prop)->data.number;
        }

        current_node = current;

        if (var_to_process > 0)
        {
            write_char (sgf_fd, ')');
        }

        if (var_to_process == stupid_num_variations ())
        {
            delete_prop_sgf (current, PROP_VARIATION_TO_PROCESS);

            continue;
        }
        else
        {
            write_char (sgf_fd, '\n');
            write_char (sgf_fd, '(');

            /* we need to do more processing on this branchpoint, either
               to do more variations or to output the ')' */
            push_int_stack (&parse_stack, current);

            /* increment the stored variation to process */
            temp_data.number = var_to_process + 1;
            add_or_set_prop_sgf (current,
                                 PROP_VARIATION_TO_PROCESS, temp_data);
        }

        rb->yield ();

        /* now we did the setup for sibling varaitions to be processed so
           do the actual outputting of a game tree branch */

        go_to_variation_sgf (var_to_process);
        output_gametree ();
    }

    current_node = saved;
    close_file (&sgf_fd);
    DEBUGF ("done outputting, file closed\n");
    return true;
}

static void
output_header_props (void)
{
    char buffer[128];

    rb->strncpy (buffer, "GM[1]FF[4]CA[UTF-8]AP[Rockbox Goban:1.0]ST[2]\n\n",
                 sizeof (buffer));
    write_file (sgf_fd, buffer, rb->strlen (buffer));

    /* board size */
    if (board_width != board_height)
    {
        rb->snprintf (buffer, sizeof (buffer), "%s[%d:%d]",
                      prop_names[PROP_SIZE], board_width, board_height);
    }
    else
    {
        rb->snprintf (buffer, sizeof (buffer), "%s[%d]",
                      prop_names[PROP_SIZE], board_width);
    }

    write_file (sgf_fd, buffer, rb->strlen (buffer));

    rb->snprintf (buffer, sizeof (buffer), "%s[", prop_names[PROP_KOMI]);
    write_file (sgf_fd, buffer, rb->strlen (buffer));

    snprint_fixed (buffer, sizeof (buffer), header.komi);
    write_file (sgf_fd, buffer, rb->strlen (buffer));

    write_char (sgf_fd, ']');

    output_header_helper (PROP_RULESET);
    output_header_helper (PROP_RESULT);

    output_header_helper (PROP_BLACK_NAME);
    output_header_helper (PROP_WHITE_NAME);
    output_header_helper (PROP_BLACK_RANK);
    output_header_helper (PROP_WHITE_RANK);
    output_header_helper (PROP_BLACK_TEAM);
    output_header_helper (PROP_WHITE_TEAM);

    output_header_helper (PROP_EVENT);
    output_header_helper (PROP_PLACE);
    output_header_helper (PROP_DATE);

    if (output_header_helper (PROP_OVERTIME) || header.time_limit != 0)
    {
        rb->snprintf (buffer, sizeof (buffer), "%s[%d]",
                      prop_names[PROP_TIME_LIMIT], header.time_limit);
        write_file (sgf_fd, buffer, rb->strlen (buffer));
    }

    write_char (sgf_fd, '\n');
    write_char (sgf_fd, '\n');
}

static bool
output_header_helper (enum prop_type_t type)
{
    char *buffer;
    int size;
    char temp_buffer[16];

    if (!get_header_string_and_size (&header, type, &buffer, &size))
    {
        DEBUGF ("output_header_helper called with invalid prop type!!\n");
        return false;
    }

    if (rb->strlen (buffer))
    {
        rb->snprintf (temp_buffer, sizeof (temp_buffer), "%s[",
                      prop_names[type]);

        write_file (sgf_fd, temp_buffer, rb->strlen (temp_buffer));

        write_file (sgf_fd, buffer, rb->strlen (buffer));

        rb->strcpy (temp_buffer, "]");

        write_file (sgf_fd, temp_buffer, rb->strlen (temp_buffer));

        return true;
    }

    return false;
}

bool first_node_in_tree = true;
static void
output_gametree (void)
{
    first_node_in_tree = true;

    while (output_current_node ())
    {
        current_node = get_node (current_node)->next;
    }

}

static bool
output_current_node (void)
{
    if (current_node < 0)
    {
        return false;
    }

    if (stupid_num_variations () > 1 &&
        get_prop_sgf (current_node, PROP_VARIATION_TO_PROCESS, NULL) < 0)
    {
        /* push it up for the gametree stuff to take care of it and fail
           out, stopping the node printing */
        push_int_stack (&parse_stack, current_node);
        return false;
    }

    if (first_node_in_tree)
    {
        first_node_in_tree = false;
    }
    else
    {
        write_char (sgf_fd, '\n');
    }
    write_char (sgf_fd, ';');

    output_all_props ();

    return true;
}

enum prop_type_t last_output_type = PROP_INVALID;
static void
output_all_props (void)
{
    int temp_handle = get_node (current_node)->props;

    last_output_type = PROP_INVALID;

    while (temp_handle >= 0)
    {
        output_prop (temp_handle);
        temp_handle = get_prop (temp_handle)->next;
    }
}

static void
output_prop (int prop_handle)
{
    char buffer[16];
    enum prop_type_t temp_type = get_prop (prop_handle)->type;

    buffer[0] = 't';
    buffer[1] = 't';

    if (is_handled_sgf (temp_type) && temp_type != PROP_COMMENT)
    {
        if (temp_type != last_output_type)
        {
            write_file (sgf_fd, prop_names[temp_type],
                        PROP_NAME_LEN (temp_type));
        }

        write_char (sgf_fd, '[');

        if (temp_type == PROP_HANDICAP)
        {
            rb->snprintf (buffer, sizeof (buffer), "%d",
                          get_prop (prop_handle)->data.number);
            write_file (sgf_fd, buffer, rb->strlen (buffer));
        }
        else if (temp_type == PROP_LABEL)
        {
            pos_to_sgf (get_prop (prop_handle)->data.position, buffer);
            buffer[2] = '\0';

            rb->snprintf (&buffer[2], sizeof (buffer) - 2, ":%c",
                          get_prop (prop_handle)->data.label_extra);

            write_file (sgf_fd, buffer, rb->strlen (buffer));
        }
        else
        {
            pos_to_sgf (get_prop (prop_handle)->data.position, buffer);

            write_file (sgf_fd, buffer, 2);
        }

        write_char (sgf_fd, ']');
    }
    else if (temp_type == PROP_ROOT_PROPS)
    {
        output_header_props ();
    }
    else if (temp_type == PROP_GENERIC_UNHANDLED || temp_type == PROP_COMMENT)
    {
        bool escaped = false;
        bool in_prop_value = false;
        int temp;
        bool done = false;

        rb->lseek (unhandled_fd, get_prop (prop_handle)->data.number,
                   SEEK_SET);

        while (!done)
        {
            temp = peek_char (unhandled_fd);

            switch (temp)
            {
            case ';':
                escaped = false;
                if (in_prop_value)
                {
                    break;
                }
                /* otherwise, fall through */
            case -1:
                done = true;
                break;

            case '\\':
                escaped = !escaped;
                break;

            case '[':
                escaped = false;
                in_prop_value = true;
                break;

            case ']':
                if (!escaped)
                {
                    in_prop_value = false;
                }
                escaped = false;
                break;

            default:
                escaped = false;
                break;
            };

            if (!done)
            {
                write_char (sgf_fd, temp);
                read_char (unhandled_fd);
            }
        }
    }

    last_output_type = temp_type;
}

static void
pos_to_sgf (unsigned short pos, char *buffer)
{
    if (pos == PASS_POS)
    {
        /* "tt" is a pass per SGF specification */
        buffer[0] = buffer[1] = 't';
    }
    else if (pos != INVALID_POS)
    {
        buffer[0] = 'a' + I (pos);
        buffer[1] = 'a' + J (pos);
    }
    else
    {
        DEBUGF ("invalid pos converted to SGF\n");
    }
}

static int
stupid_num_variations (void)
{
    int result = 1;
    struct prop_t *temp_prop;
    struct node_t *temp_node = get_node (current_node);

    if (temp_node == 0)
    {
        return 0;
    }

    temp_prop = get_prop (temp_node->props);

    while (temp_prop)
    {
        if (temp_prop->type == PROP_VARIATION)
        {
            ++result;
        }
        else
        {
            // variations are at the beginning of the prop list
            break;
        }

        temp_prop = get_prop (temp_prop->next);
    }

    return result;
}

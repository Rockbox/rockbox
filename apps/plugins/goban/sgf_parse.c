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
#include "sgf_parse.h"
#include "sgf.h"
#include "sgf_storage.h"
#include "util.h"
#include "board.h"
#include "game.h"

static void handle_prop_value (enum prop_type_t type);
static int read_prop_value (char *buffer, size_t buffer_size);
static void do_range (enum prop_type_t type, unsigned short ul,
                      unsigned short br);
static void parse_prop (void);
static void parse_node (void);
static enum prop_type_t parse_prop_type (void);

static unsigned short sgf_to_pos (char *buffer);

bool
parse_sgf (const char *filename)
{
    int saved = current_node;

    /* for parsing */
    int first_handle = 0;       /* first node in the branch */
    int file_position = 0;

    int temp;

    close_file (&sgf_fd);

    sgf_fd = rb->open (filename, O_RDONLY);

    if (sgf_fd < 0)
    {
        return false;
    }

    current_node = start_node;

    if (current_node < 0)
    {
        current_node = saved;
        return false;
    }

    empty_stack (&parse_stack);

    /* seek to the first '(' */
    while (peek_char_no_whitespace (sgf_fd) != '(')
    {
        if (read_char_no_whitespace (sgf_fd) == -1)
        {
            DEBUGF ("end of file or error before we found a '('\n");
            current_node = saved;
            return false;
        }
    }

    push_int_stack (&parse_stack, rb->lseek (sgf_fd, 0, SEEK_CUR));
    push_int_stack (&parse_stack, current_node);

    while (pop_int_stack (&parse_stack, &first_handle) &&
           pop_int_stack (&parse_stack, &file_position))
    {
        /* DEBUGF("poped off %d\n", file_position); */

        rb->yield ();

        current_node = first_handle;

        if (file_position == -1)
        {
            temp = read_char_no_whitespace (sgf_fd);
            if (temp != '(')
            {
                /* we're here because there may have been a sibling after
                   another gametree that was handled, but there's no '(',
                   so there wasnt' a sibling, so just go on to any more
                   gametrees in the stack */
                continue;
            }
            else
            {
                /* there may be more siblings after we process this one */
                push_int_stack (&parse_stack, -1);
                push_int_stack (&parse_stack, first_handle);
            }
        }
        else
        {
            /* check for a sibling after we finish with this node */
            push_int_stack (&parse_stack, -1);
            push_int_stack (&parse_stack, first_handle);

            rb->lseek (sgf_fd, file_position, SEEK_SET);


            /* we're at the start of a gametree here, right at the '(' */
            temp = read_char_no_whitespace (sgf_fd);

            if (temp != '(')
            {
                DEBUGF ("start of gametree doesn't have a '('!\n");
                current_node = saved;
                return false;
            }
        }

        while (1)
        {
            temp = peek_char_no_whitespace (sgf_fd);
            /* DEBUGF("||| %d, %c\n", absolute_position(), (char) temp); */

            if (temp == ';')
            {
                /* fill the tree_head node before moving on */
                if (current_node != tree_head ||
                    get_node (current_node)->props >= 0)
                {
                    int temp = add_child_sgf (NULL);

                    if (temp >= 0)
                    {
                        current_node = temp;
                    }
                    else
                    {
                        rb->splash (2 * HZ, "Out of memory while parsing!");
                        return false;
                    }
                }


                read_char_no_whitespace (sgf_fd);
                parse_node ();
            }
            else if (temp == ')')
            {
                /* finished this gametree */

                /* we want to end one past the ')', so eat it up: */
                read_char_no_whitespace (sgf_fd);
                break;
            }
            else if (temp == '(')
            {
                /*
                   DEBUGF ("adding %d\n", (int) rb->lseek (sgf_fd, 0,
                   SEEK_CUR)); */
                push_int_stack (&parse_stack, rb->lseek (sgf_fd, 0, SEEK_CUR));
                push_int_stack (&parse_stack, current_node);

                break;
            }
            else if (temp == -1)
            {
                break;
            }
            else
            {
                DEBUGF ("extra characters found while parsing: %c\n", temp);
                /* skip the extras i guess */
                read_char_no_whitespace (sgf_fd);
            }
        }
    }

    current_node = get_node (tree_head)->next;
    while (current_node >= 0 && get_node (current_node)->props < 0)
    {
        temp = current_node;    /* to be freed later */

        /* update the ->prev pointed on all branches of the next node */
        current_node = get_node (current_node)->next;
        /* DEBUGF("trying to set prev for branch %d\n", current_node); */
        if (current_node >= 0)
        {
            get_node (current_node)->prev = tree_head;

            struct prop_t *loop_prop =
                get_prop (get_node (current_node)->props);

            while (loop_prop != 0)
            {
                if (loop_prop->type == PROP_VARIATION)
                {
                    get_node (loop_prop->data.number)->prev = tree_head;
                }
                else
                {
                    /* all of the variations have to be up front, so we
                       can quit here */
                    break;
                }
                loop_prop = get_prop (loop_prop->next);
            }
        }

        /* update the tree head */
        get_node (tree_head)->next = get_node (temp)->next;
        /* DEBUGF("freeing %d %d %d\n", temp, start_node, saved); */
        if (start_node == temp || saved == temp)
        {
            start_node = saved = tree_head;
        }
        free_storage_sgf (temp);

        current_node = get_node (tree_head)->next;
    }

    current_node = saved;


    /* DEBUGF("got past!\n"); */
    close_file (&sgf_fd);
    return true;
}


static void
parse_node (void)
{
    int temp;

    while (1)
    {
        temp = peek_char_no_whitespace (sgf_fd);

        if (temp == -1 || temp == ')' || temp == '(' || temp == ';')
        {
            return;
        }
        else
        {
            parse_prop ();
        }
    }
}



int start_of_prop = 0;
static void
parse_prop (void)
{
    enum prop_type_t temp_type = PROP_INVALID;
    int temp;


    while (1)
    {
        temp = peek_char_no_whitespace (sgf_fd);

        if (temp == -1 || temp == ')' || temp == '(' || temp == ';')
        {
            return;
        }
        else if (temp == '[')
        {
            handle_prop_value (temp_type);
        }
        else
        {
            start_of_prop = rb->lseek (sgf_fd, 0, SEEK_CUR);
            temp_type = parse_prop_type ();
        }
    }
}

static enum prop_type_t
parse_prop_type (void)
{
    char buffer[3];
    int pos = 0;
    int temp;

    rb->memset (buffer, 0, sizeof (buffer));

    while (1)
    {
        temp = peek_char_no_whitespace (sgf_fd);

        if (temp == ';' || temp == '[' || temp == '(' ||
            temp == -1 || temp == ')')
        {
            if (pos == 1 || pos == 2)
            {
                break;
            }
            else
            {
                return PROP_INVALID;
            }
        }
        else if (temp >= 'A' && temp <= 'Z')
        {
            buffer[pos++] = temp;

            if (pos == 2)
            {
                read_char_no_whitespace (sgf_fd);
                break;
            }
        }

        temp = read_char_no_whitespace (sgf_fd);
    }

    /* check if we're still reading a prop name, in which case we fail
       (but first we want to eat up the rest of the prop name) */
    bool failed = false;
    while (peek_char_no_whitespace (sgf_fd) != ';' &&
           peek_char_no_whitespace (sgf_fd) != '[' &&
           peek_char_no_whitespace (sgf_fd) != '(' &&
           peek_char_no_whitespace (sgf_fd) != '}' &&
           peek_char_no_whitespace (sgf_fd) != -1)
    {
        failed = true;
        read_char_no_whitespace (sgf_fd);
    }

    if (failed)
    {
        return PROP_INVALID;
    }

    int i;
    for (i = 0; i < PROP_NAMES_SIZE; ++i)
    {
        if (rb->strcmp (buffer, prop_names[i]) == 0)
        {
            return (enum prop_type_t) i;
        }
    }
    return PROP_INVALID;
}

static int
read_prop_value (char *buffer, size_t buffer_size)
{
    bool escaped = false;
    int temp;
    int bytes_read = 0;

    /* make it a string, the lazy way */
    rb->memset (buffer, 0, buffer_size);
    --buffer_size;

    if (peek_char (sgf_fd) == '[')
    {
        read_char (sgf_fd);
    }

    while (1)
    {
        temp = read_char (sgf_fd);
        if (temp == ']' && !escaped)
        {
            return bytes_read;
        }
        else if (temp == '\\')
        {
            if (escaped)
            {
                if (buffer && buffer_size)
                {
                    *(buffer++) = temp;
                    ++bytes_read;
                    --buffer_size;
                }
            }
            escaped = !escaped;
        }
        else if (temp == -1)
        {
            return bytes_read;
        }
        else
        {
            escaped = false;
            if (buffer && buffer_size)
            {
                *(buffer++) = temp;
                ++bytes_read;
                --buffer_size;
            }
        }
    }
}

static void
handle_prop_value (enum prop_type_t type)
{
    /* max size of generically supported prop values is 6, which is 5 for
       a point range ab:cd and one for the \0

       (this buffer is only used for them, things such as white and black
       player names are stored in different buffers) */

    /* make it a little bigger for other random crap, like reading in time
     */
#define PROP_HANDLER_BUFFER_SIZE 16

    char real_buffer[PROP_HANDLER_BUFFER_SIZE];
    char *buffer = real_buffer;

    int temp;
    union prop_data_t temp_data;
    bool in_prop_value = false;
    bool escaped = false;
    bool done = false;
    int temp_width, temp_height;
    unsigned short temp_pos_ul, temp_pos_br;
    int temp_size;
    char *temp_buffer;
    bool got_value;

    /* special extra handling for root properties, set a marker telling us
       the right place to spit the values out in output_sgf */
    if (type == PROP_GAME ||
        type == PROP_APPLICATION ||
        type == PROP_CHARSET ||
        type == PROP_SIZE ||
        type == PROP_FILE_FORMAT || type == PROP_VARIATION_TYPE)
    {
        header_marked = true;

        temp_data.number = 0;   /* meaningless */

        /* don't add more than one, so just set it if we found one already
         */
        add_or_set_prop_sgf (current_node, PROP_ROOT_PROPS, temp_data);
    }


    if (!is_handled_sgf (type) || type == PROP_COMMENT)
    {
        /* DEBUGF("unhandled prop %d\n", (int) type); */
        rb->lseek (sgf_fd, start_of_prop, SEEK_SET);

        temp_data.number = rb->lseek (unhandled_fd, 0, SEEK_CUR);
        /* absolute_position(&unhandled_prop_list); */

        add_prop_sgf (current_node,
                      type == PROP_COMMENT ? PROP_COMMENT :
                      PROP_GENERIC_UNHANDLED, temp_data);

        got_value = false;
        while (!done)
        {
            temp = peek_char (sgf_fd);

            switch (temp)
            {
            case -1:
                done = true;
                break;

            case '\\':
                if (got_value && !in_prop_value)
                {
                    done = true;
                }
                escaped = !escaped;
                break;
            case '[':
                escaped = false;
                in_prop_value = true;
                got_value = true;
                break;
            case ']':
                if (!escaped)
                {
                    in_prop_value = false;
                }
                escaped = false;
                break;
            case ')':
            case '(':
            case ';':
                if (!in_prop_value)
                {
                    done = true;
                }
                escaped = false;
                break;
            default:
                if (got_value && !in_prop_value)
                {
                    if (!is_whitespace (temp))
                    {
                        done = true;
                    }
                }
                escaped = false;
                break;
            };

            if (done)
            {
                write_char (unhandled_fd, ';');
            }
            else
            {
                /* don't write out-of-prop whitespace */
                if (in_prop_value || !is_whitespace (temp))
                {
                    write_char (unhandled_fd, (char) temp);
                }

                read_char (sgf_fd);
            }
        }


        return;
    }
    else if (type == PROP_BLACK_MOVE || type == PROP_WHITE_MOVE)
    {
        /* DEBUGF("move prop %d\n", (int) type); */

        temp = read_prop_value (buffer, PROP_HANDLER_BUFFER_SIZE);

        temp_data.position = INVALID_POS;

        /* empty is apparently acceptable as a pass */
        if (temp == 0)
        {
            temp_data.position = PASS_POS;
        }
        else if (temp == 2)
        {
            temp_data.position = sgf_to_pos (buffer);
        }
        else
        {
            DEBUGF ("invalid move position read in, of wrong size!\n");
        }


        if (temp_data.position != INVALID_POS)
        {
            add_prop_sgf (current_node, type, temp_data);
        }

        return;
    }
    else if (type == PROP_ADD_BLACK ||
             type == PROP_ADD_WHITE ||
             type == PROP_ADD_EMPTY ||
             type == PROP_CIRCLE ||
             type == PROP_SQUARE ||
             type == PROP_TRIANGLE ||
             type == PROP_DIM || type == PROP_MARK || type == PROP_SELECTED)
    {
        /* DEBUGF("add prop %d\n", (int) type); */

        temp = read_prop_value (buffer, PROP_HANDLER_BUFFER_SIZE);
        if (temp == 2)
        {
            temp_data.position = sgf_to_pos (buffer);

            if (temp_data.position != INVALID_POS &&
                temp_data.position != PASS_POS)
            {
                add_prop_sgf (current_node, type, temp_data);
            }
        }
        else if (temp == 5)
        {
            /* example: "ab:cd", two positions separated by a colon */
            temp_pos_ul = sgf_to_pos (buffer);
            temp_pos_br = sgf_to_pos (&(buffer[3]));

            if (!on_board (temp_pos_ul) || !on_board (temp_pos_br) ||
                buffer[2] != ':')
            {
                DEBUGF ("invalid range value!\n");
            }

            do_range (type, temp_pos_ul, temp_pos_br);
        }
        else
        {
            DEBUGF ("invalid position or range read in. wrong size!\n");
        }
        return;
    }
    else if (type == PROP_LABEL)
    {
        temp = read_prop_value (buffer, PROP_HANDLER_BUFFER_SIZE);

        if (temp < 4 || buffer[2] != ':')
        {
            DEBUGF ("invalid LaBel property '%s'", buffer);
        }
        temp_data.position = sgf_to_pos (buffer);

        if (!on_board (temp_data.position))
        {
            DEBUGF ("LaBel set on invalid position!\n");
        }

        temp_data.label_extra = buffer[3];

        add_prop_sgf (current_node, type, temp_data);
        return;
    }
    else if (type == PROP_GAME)
    {
        temp = read_prop_value (buffer, PROP_HANDLER_BUFFER_SIZE);
        if (temp != 1 || buffer[0] != '1')
        {
            rb->splash (2 * HZ, "This isn't a Go SGF!!  Parsing stopped.");
            DEBUGF ("incorrect game type loaded!\n");

            close_file (&sgf_fd);
        }
    }
    else if (type == PROP_FILE_FORMAT)
    {
        temp = read_prop_value (buffer, PROP_HANDLER_BUFFER_SIZE);
        if (temp != 1 || (buffer[0] != '3' && buffer[0] != '4'))
        {
            rb->splash (2 * HZ, "Wrong SGF file version! Parsing stopped.");
            DEBUGF ("can't handle file format %c\n", buffer[0]);

            close_file (&sgf_fd);
        }
    }
    else if (type == PROP_APPLICATION ||
             type == PROP_CHARSET || type == PROP_VARIATION_TYPE)
    {
        /* we don't care.  on output we'll write our own values for these */
        read_prop_value (NULL, 0);
    }
    else if (type == PROP_SIZE)
    {
        temp = read_prop_value (buffer, PROP_HANDLER_BUFFER_SIZE);
        if (temp == 0)
        {
            rb->splash (HZ, "Invalid board size specified in file.");
        }
        else
        {
            temp_width = rb->atoi (buffer);
            while (*buffer != ':' && *buffer != '\0')
            {
                ++buffer;
            }

            if (*buffer != '\0')
            {
                ++buffer;
                temp_height = rb->atoi (buffer);
            }
            else
            {
                temp_height = temp_width;
            }


            if (!set_size_board (temp_width, temp_height))
            {
                rb->splashf (HZ,
                             "Board too big/small! (%dx%d) Stopping parse.",
                             temp_width, temp_height);
                close_file (&sgf_fd);
            }
            else
            {
                clear_board ();
            }
        }
    }
    else if (type == PROP_KOMI)
    {
        temp = read_prop_value (buffer, PROP_HANDLER_BUFFER_SIZE);

        if (temp == 0)
        {
            header.komi = 0;
            DEBUGF ("invalid komi specification. setting to zero\n");
        }
        else
        {
            header.komi = rb->atoi (buffer) << 1;
            while (*buffer != '.' && *buffer != ',' && *buffer != '\0')
            {
                ++buffer;
            }

            if (buffer != '\0')
            {
                ++buffer;

                if (*buffer == 0)
                {
                    /* do nothing */
                }
                else if (*buffer >= '1' && *buffer <= '9')
                {
                    header.komi += 1;
                }
                else
                {
                    if (*buffer != '0')
                    {
                        DEBUGF ("extra characters after komi value!\n");
                    }
                }
            }
        }
    }
    else if (type == PROP_BLACK_NAME ||
             type == PROP_WHITE_NAME ||
             type == PROP_BLACK_RANK ||
             type == PROP_WHITE_RANK ||
             type == PROP_BLACK_TEAM ||
             type == PROP_WHITE_TEAM ||
             type == PROP_DATE ||
             type == PROP_ROUND ||
             type == PROP_EVENT ||
             type == PROP_PLACE ||
             type == PROP_OVERTIME ||
             type == PROP_RESULT || type == PROP_RULESET)
    {
        if (!get_header_string_and_size
            (&header, type, &temp_buffer, &temp_size))
        {
            rb->splash (5 * HZ,
                        "Error getting header string.  Report this.");
        }
        else
        {
            temp = read_prop_value (temp_buffer, temp_size - 1);
#if 0
            DEBUGF ("read %d bytes into header for type: %d\n", temp, type);
            DEBUGF ("data: %s\n", temp_buffer);
#endif
        }
    }
    else if (type == PROP_TIME_LIMIT)
    {
        temp = read_prop_value (buffer, PROP_HANDLER_BUFFER_SIZE);
        header.time_limit = rb->atoi (buffer);
        DEBUGF ("setting time: %d (%s)\n", header.time_limit, buffer);
    }
    else if (type == PROP_HANDICAP)
    {
        temp = read_prop_value (buffer, PROP_HANDLER_BUFFER_SIZE);
        if (start_node == tree_head)
        {
            if (rb->atoi (buffer) >= 2)
            {
                start_node = current_node;
                temp_data.number = header.handicap = rb->atoi (buffer);
                add_prop_sgf (current_node, type, temp_data);
                DEBUGF ("setting handicap: %d\n", header.handicap);
            }
            else
            {
                DEBUGF ("invalid HAndicap prop.  ignoring\n");
            }
        }
        else
        {
            rb->splash (HZ, "extraneous HAndicap prop present in file!\n");
        }
    }
    else
    {
        DEBUGF ("UNHANDLED PROP TYPE!!!\n");
        rb->splash (3 * HZ,
                    "A SGF prop was not dealt with.  Please report this");
        read_prop_value (NULL, 0);
    }
}



/* upper-left and bottom right */
static void
do_range (enum prop_type_t type, unsigned short ul, unsigned short br)
{
    /* this code is overly general and accepts ranges even if ul and br
       aren't the required corners it's easier doing that that failing if
       the input is bad */

    bool x_reverse = false;
    bool y_reverse = false;
    union prop_data_t temp_data;

    if (I (br) < I (ul))
    {
        x_reverse = true;
    }

    if (J (br) < J (ul))
    {
        y_reverse = true;
    }

    int x, y;
    for (x = I (ul);
         x_reverse ? (x >= I (br)) : (x <= I (br)); x_reverse ? --x : ++x)
    {
        for (y = J (ul);
             y_reverse ? (y >= J (br)) : (y <= J (br)); y_reverse ? --y : ++y)
        {
            temp_data.position = POS (x, y);

            DEBUGF ("adding %d %d for range (type %d)\n",
                    I (temp_data.position), J (temp_data.position), type);
            add_prop_sgf (current_node, type, temp_data);
        }
    }
}



static unsigned short
sgf_to_pos (char *buffer)
{
    if (buffer[0] == 't' && buffer[1] == 't')
    {
        return PASS_POS;
    }
    else if (buffer[0] < 'a' || buffer[0] > 'z' ||
             buffer[1] < 'a' || buffer[1] > 'z')
    {
        return INVALID_POS;
    }
    return POS (buffer[0] - 'a', buffer[1] - 'a');
}


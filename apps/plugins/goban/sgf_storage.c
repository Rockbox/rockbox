
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
#include "sgf_storage.h"
#include "sgf.h"
#include "util.h"

#define ALIGNMENT_VAL (sizeof (union storage_t))

union storage_t *storage_buffer[] = { NULL, NULL };
size_t storage_buffer_size[] = { 0, 0 };

uint8_t *storage_free_list[] = { NULL, NULL };
size_t storage_free_list_size[] = { 0, 0 };

bool storage_initialized[] = { false, false };

size_t total_storage_size = 0;

/* the next handle to check */
int next_free_handle_buffer;
int next_free_handle;

static bool setup_storage_buffer (char *temp_buffer, size_t size);
static void clear_storage_buffer (int index);
static bool find_free (int *ret_buffer, int *ret_handle);
static bool is_free (int buffer_num, int handle);
static void set_free (int buffer_num, int handle, bool free);

#if 0
static void debugf_current_node (void);

static void
debugf_current_node (void)
{
    int temp_prop = NO_PROP;
    if (current_node < 0)
    {
        DEBUGF ("CURRENT_NODE < 0 ON DEBUGF_CURRENT_NODE!!!!\n");
        return;
    }
    DEBUGF ("-----------------------------------------\n");
    DEBUGF ("current_node: %d\n", current_node);
    DEBUGF ("start_node %d %d\n", start_node, tree_head);
    DEBUGF ("prev/next: %d/%d\n", get_node (current_node)->prev,
            get_node (current_node)->next);
    DEBUGF ("num variations: %d\n", num_variations_sgf ());
    DEBUGF ("props:\n");
    if (!get_node (current_node) ||
        (temp_prop = get_node (current_node)->props) < 0)
    {
        DEBUGF ("none\n");
    }

    while (1)
    {
        if (temp_prop < 0)
        {
            break;
        }
        DEBUGF ("  handle: %d\n", temp_prop);
        DEBUGF ("  type: %d ", get_prop (temp_prop)->type);
        if (get_prop (temp_prop)->type < PROP_NAMES_SIZE)
        {
            DEBUGF ("(%s)", prop_names[get_prop (temp_prop)->type]);
        }
        DEBUGF ("\n");
        if (get_prop (temp_prop)->type == PROP_BLACK_MOVE ||
            get_prop (temp_prop)->type == PROP_WHITE_MOVE ||
            get_prop (temp_prop)->type == PROP_ADD_BLACK ||
            get_prop (temp_prop)->type == PROP_ADD_WHITE ||
            get_prop (temp_prop)->type == PROP_ADD_EMPTY)
        {
            DEBUGF ("  i: %d  j: %d\n",
                    I (get_prop (temp_prop)->data.position),
                    J (get_prop (temp_prop)->data.position));
        }
        else
        {
            DEBUGF ("  data: %d\n", get_prop (temp_prop)->data.number);
        }
        DEBUGF ("  next: %d\n", get_prop (temp_prop)->next);

        temp_prop = get_prop (temp_prop)->next;
        if (temp_prop >= 0)
        {
            DEBUGF ("\n");
        }
    }

    DEBUGF ("-----------------------------------------\n");
}
#endif

static void
clear_storage_buffer (int index)
{
    int temp;


    /* everything starts free */
    rb->memset (storage_free_list[index],
                (unsigned char) 0xFF,
                storage_free_list_size[index]);

    /* if there are extra bits at the end of the free list (because
       storage_buffer_size is not divisible by 8) then we set them not
       free, so we won't end up using those ever by accident (shouldn't be
       possible anyways, but makes calculation easier later) */
    temp = storage_free_list_size[index] * 8 - storage_buffer_size[index];
    storage_free_list[index][storage_free_list_size[index] - 1] ^=
        (1 << temp) - 1;
}

void
free_tree_sgf (void)
{
    unsigned int i;
    for (i = 0;
         i < sizeof (storage_initialized) /
         sizeof (storage_initialized[0]); ++i)
    {
        if (storage_initialized[i])
        {
            clear_storage_buffer (i);
        }
    }

    tree_head = start_node = current_node = alloc_storage_sgf ();

    if (tree_head < 0)
    {
        rb->splash (5 * HZ,
                    "Error allocating first node!  Please exit immediately.");
    }

    get_node (tree_head)->props = NO_PROP;
    get_node (tree_head)->next = NO_NODE;
    get_node (tree_head)->prev = NO_NODE;
}


bool
audio_stolen_sgf (void)
{
    return storage_initialized[1];
}

int
alloc_storage_sgf (void)
{
    int buffer_num;
    int handle;
    int temp_buffer;
    int ret_val;

    char *new_storage_buffer;
    size_t size;

    if (!find_free (&buffer_num, &handle))
    {
        if (!storage_initialized[1])
        {
            rb->splash (2 * HZ, "Stopping music playback to get more space");
            DEBUGF ("stealing audio buffer: %d\n", (int) total_storage_size);

            new_storage_buffer = rb->plugin_get_audio_buffer (&size);
            setup_storage_buffer (new_storage_buffer, size);

            DEBUGF ("after stealing: %d\n", (int) total_storage_size);
        }
        else
        {
            return -1;
        }

        /* try again */
        if (!find_free (&buffer_num, &handle))
        {
            return -1;
        }
    }

    set_free (buffer_num, handle, false);

    temp_buffer = 0;
    ret_val = handle;

    while (temp_buffer != buffer_num)
    {
        if (storage_initialized[temp_buffer])
        {
            ret_val += storage_buffer_size[temp_buffer];
        }

        ++temp_buffer;
    }

    return ret_val;
}

void
free_storage_sgf (int handle)
{
    int index;

    if (handle < 0 || (unsigned int) handle >= total_storage_size)
    {
        DEBUGF ("tried to free an out of bounds handle!!\n");
    }
    else
    {
        index = 0;
        while ((unsigned int) handle >= storage_buffer_size[index])
        {
            handle -= storage_buffer_size[index++];
        }
        rb->memset (&storage_buffer[index][handle], 0xFF,
                    sizeof (union storage_t));
        set_free (index, handle, true);
    }
}

static bool
find_free (int *ret_buffer, int *ret_handle)
{
    unsigned int handle = next_free_handle;
    unsigned int buffer_index = next_free_handle_buffer;

    /* so we know where we started, to prevent infinite loop */
    unsigned int start_handle = handle;
    unsigned int start_buffer = buffer_index;


    do
    {
        ++handle;

        if (handle >= storage_buffer_size[buffer_index])
        {
            handle = 0;

            do
            {
                ++buffer_index;

                if (buffer_index >= sizeof (storage_initialized) /
                    sizeof (storage_initialized[0]))
                {
                    buffer_index = 0;
                }
            }
            while (!storage_initialized[buffer_index]);

        }

        if (is_free (buffer_index, handle))
        {
            next_free_handle_buffer = buffer_index;
            next_free_handle = handle;

            *ret_buffer = buffer_index;
            *ret_handle = handle;

            return true;
        }
    }
    while (handle != start_handle || buffer_index != start_buffer);

    return false;
}

static bool
is_free (int buffer_num, int handle)
{
    return storage_free_list[buffer_num][handle / 8] &
        (1 << (7 - (handle % 8)));
}

static void
set_free (int buffer_num, int handle, bool free)
{
    if (free)
    {
        /* simple, just 'or' the byte with the specific bit switched on */
        storage_free_list[buffer_num][handle / 8] |= 1 << (7 - (handle % 8));
    }
    else
    {
        /* start with a byte with all bits turned on and turn off the one
           we're trying to set to zero.  then take that result and 'and'
           it with the current value */
        storage_free_list[buffer_num][handle / 8] &=
            0xFF ^ (1 << (7 - (handle % 8)));
    }
}

bool
setup_sgf (void)
{
    size_t size;
    char *temp_buffer;

    temp_buffer = rb->plugin_get_buffer (&size);
    setup_storage_buffer (temp_buffer, size);

    if (total_storage_size < MIN_STORAGE_BUFFER_SIZE)
    {
        rb->splash (2 * HZ, "Stopping music playback to get more space");
        DEBUGF ("storage_buffer_size < MIN!!: %d\n", (int) total_storage_size);

        temp_buffer = rb->plugin_get_audio_buffer (&size);
        setup_storage_buffer (temp_buffer, size);
    }

    if (total_storage_size < MIN_STORAGE_BUFFER_SIZE)
    {
        rb->splash (5 * HZ, "Low memory.  Large files may not load.");

        DEBUGF ("storage_buffer_size < MIN!!!!: %d\n",
                (int) total_storage_size);
    }

    DEBUGF ("storage_buffer_size: %d\n", (int) total_storage_size);


    unhandled_fd = create_or_open_file (UNHANDLED_PROP_LIST_FILE);

    if (unhandled_fd < 0)
    {
        return false;
    }

    rb->lseek (unhandled_fd, 0, SEEK_SET);
    rb->ftruncate (unhandled_fd, 0);

    empty_stack (&parse_stack);

    return true;
}


void
clear_caches_sgf (void)
{
    empty_stack (&parse_stack);

    rb->lseek (unhandled_fd, 0, SEEK_SET);
    rb->ftruncate (unhandled_fd, 0);
}

void
cleanup_sgf (void)
{
    empty_stack (&parse_stack);

    rb->lseek (unhandled_fd, 0, SEEK_SET);
    rb->ftruncate (unhandled_fd, 0);
    close_file (&unhandled_fd);

    close_file (&sgf_fd);
}

static bool
setup_storage_buffer (char *temp_buffer, size_t size)
{
    unsigned int index = 0;
    int temp;

    while (1)
    {
        if (index >= sizeof (storage_initialized) /
            sizeof (storage_initialized[0]))
        {
            return false;
        }

        if (!storage_initialized[index])
        {
            break;
        }
        ++index;
    }

    temp_buffer = align_buffer (temp_buffer, &size);
    if (!temp_buffer || !size)
    {
        return false;
    }

    /* same as temp = size / (sizeof(union storage_t) + 1/8)
       (we need 1 bit extra for each union storage_t, for the free list) */
    temp =
        (8 * (size - ALIGNMENT_VAL - 1)) / (8 * sizeof (union storage_t) + 1);
    /* the - ALIGNMENT_VAL - 1 is for possible wasted space in alignment
       and possible extra byte needed in the free list */

    storage_buffer[index] = (void *) temp_buffer;
    storage_buffer_size[index] = temp;

    storage_free_list_size[index] = storage_buffer_size[index] / 8;
    if (storage_free_list_size[index] * 8 < storage_buffer_size[index])
    {
        ++(storage_free_list_size[index]);
    }

    temp_buffer += sizeof (union storage_t) * temp;
    size -= sizeof (union storage_t) * temp;

    temp_buffer = align_buffer (temp_buffer, &size);
    if (!temp_buffer || !size)
    {
        return false;
    }

    if (size < storage_free_list_size[index])
    {
        DEBUGF ("Big problem on line %d in sgf.c\n", __LINE__);
        rb->splashf (5 * HZ,
                "Error in allocating storage buffer! Exit and report this!\n");
        return false;
    }

    storage_free_list[index] = temp_buffer;
    total_storage_size += storage_buffer_size[index];
    storage_initialized[index] = true;

    clear_storage_buffer (index);

    return true;
}

struct node_t *
get_node (int handle)
{
    if (handle < 0)
    {
        return NULL;
    }
    else if ((unsigned int) handle >= total_storage_size)
    {
        return NULL;
    }

    int index = 0;
    while ((unsigned int) handle >= storage_buffer_size[index])
    {
        handle -= storage_buffer_size[index++];
    }
    return &(storage_buffer[index][handle].node);
}

struct prop_t *
get_prop (int handle)
{
    if (handle < 0)
    {
        return NULL;
    }
    else if ((unsigned int) handle >= total_storage_size)
    {
        return NULL;
    }

    int index = 0;
    while ((unsigned int) handle >= storage_buffer_size[index])
    {
        handle -= storage_buffer_size[index++];
    }
    return &(storage_buffer[index][handle].prop);
}


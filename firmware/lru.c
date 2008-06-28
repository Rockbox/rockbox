/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2003 Tat Tang
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

#include "lru.h"

struct lru_node
{
    short _next;
    short _prev;
    unsigned char data[1]; /* place holder */
};

#define lru_node_p(pl, x) \
    ((struct lru_node*)(pl->_base + pl->_slot_size * x))

/*******************************************************************************
 * lru_create
 ******************************************************************************/
void lru_create(struct lru* pl, void *buf, short size, short data_size)
{
    pl->_base = (unsigned char*) buf;
    pl->_slot_size = data_size + LRU_SLOT_OVERHEAD;

    pl->_size = size;

    pl->_head = 0;
    pl->_tail = pl->_size - 1;

    /* Initialise slots */
    int i;
    for (i=0; i<pl->_size; i++)
    {
        lru_node_p(pl, i)->_next = i + 1;
        lru_node_p(pl, i)->_prev = i - 1;
    }

    /* Fix up head and tail to form circular buffer */
    lru_node_p(pl, 0)->_prev = pl->_tail;
    lru_node_p(pl, pl->_tail)->_next = pl->_head;
}

/*******************************************************************************
 * lru_traverse
 ******************************************************************************/
void lru_traverse(struct lru* pl, void (*callback)(void* data))
{
    short i;
    struct lru_node* slot;
    short loc = pl->_head;

    for (i = 0; i < pl->_size; i++)
    {
        slot = lru_node_p(pl, loc);
        callback(slot->data);
        loc = slot->_next;
    }
}

/*******************************************************************************
 * lru_touch
 ******************************************************************************/
void lru_touch(struct lru* pl, short handle)
{
    if (handle == pl->_head)
    {
        pl->_head = lru_node_p(pl, pl->_head)->_next;
        pl->_tail = lru_node_p(pl, pl->_tail)->_next;
        return;
    }

    if (handle == pl->_tail)
    {
        return;
    }

    /* Remove current node from linked list */
    struct lru_node* curr_node = lru_node_p(pl, handle);
    struct lru_node* prev_node = lru_node_p(pl, curr_node->_prev);
    struct lru_node* next_node = lru_node_p(pl, curr_node->_next);

    prev_node->_next = curr_node->_next;
    next_node->_prev = curr_node->_prev;

    /* insert current node at tail */
    struct lru_node* tail_node = lru_node_p(pl, pl->_tail);
    short tail_node_next_handle = tail_node->_next; /* Bug fix */
    struct lru_node* tail_node_next = lru_node_p(pl, tail_node_next_handle); /* Bug fix */

    curr_node->_next = tail_node->_next;
    curr_node->_prev = pl->_tail;
    tail_node_next->_prev = handle; /* Bug fix */
    tail_node->_next = handle;

    pl->_tail = handle;
}

/*******************************************************************************
 * lru_data
 ******************************************************************************/
void *lru_data(struct lru* pl, short handle)
{
    return lru_node_p(pl, handle)->data;
}


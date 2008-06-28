/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Linked list API definitions
 *
 * Copyright (c) 2007 Michael Sevakis
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
#include "plugin.h"
#include "mpegplayer.h"
#include "mpeg_linkedlist.h"

/* Initialize a master list head */
void list_initialize(struct list_item *master_list_head)
{
    master_list_head->prev = master_list_head->next = NULL;
}

/* Are there items after the head item? */
bool list_is_empty(struct list_item *head_item)
{
    return head_item->next == NULL;
}

/* Does the item belong to a list? */
bool list_is_item_listed(struct list_item *item)
{
    return item->prev != NULL;
}

/* Is the item a member in a particular list? */
bool list_is_member(struct list_item *master_list_head,
                    struct list_item *item)
{
    if (item != master_list_head && item->prev != NULL)
    {
        struct list_item *curr = master_list_head->next;

        while (curr != NULL)
        {
            if (item != curr)
            {
                curr = curr->next;
                continue;
            }

            return true;
        }
    }

    return false;
}

/* Remove an item from a list - no head item needed */
void list_remove_item(struct list_item *item)
{
    if (item->prev == NULL)
    {
        /* Not in a list - no change - could be the master list head
         * as well which cannot be removed */
        return;
    }

    item->prev->next = item->next;

    if (item->next != NULL)
    {
        /* Not last item */
        item->next->prev = item->prev;
    }

    /* Mark as not in a list */
    item->prev = NULL;
}

/* Add a list item after the base item */
void list_add_item(struct list_item *head_item,
                   struct list_item *item)
{
    if (item->prev != NULL)
    {
        /* Already in a list - no change */
        DEBUGF("list_add_item: item already in a list\n");
        return;
    }

    if (item == head_item)
    {
        /* Cannot add the item to itself */
        DEBUGF("list_add_item: item == head_item\n");
        return;
    }

    /* Insert first */
    item->prev = head_item;
    item->next = head_item->next;

    if (head_item->next != NULL)
    {
        /* Not first item */
        head_item->next->prev = item;
    }

    head_item->next = item;
}

/* Clear list items after the head item */
void list_clear_all(struct list_item *head_item)
{
    struct list_item *curr = head_item->next;

    while (curr != NULL)
    {
        list_remove_item(curr);
        curr = head_item->next;
    }
}

/* Enumerate all items after the head item - passing each item in turn
 * to the callback as well as the data value. The current item may be
 * safely removed. Items added after the current position will be enumated
 * but not ones added before it. The callback may return false to stop
 * the enumeration. */
void list_enum_items(struct list_item *head_item,
                     list_enum_callback_t callback,
                     intptr_t data)
{
    struct list_item *next = head_item->next;

    while (next != NULL)
    {
        struct list_item *curr = next;
        next = curr->next;
        if (!callback(curr, data))
            break;
    }
}

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Linked list API declarations
 *
 * Copyright (c) 2007 Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef MPEG_LINKEDLIST_H
#define MPEG_LINKEDLIST_H

struct list_item
{
    struct list_item *prev; /* previous item in list */
    struct list_item *next; /* next item in list */
};

/* Utility macros to help get the actual structure pointer back */
#define OFFSETOF(type, membername) ((off_t)&((type *)0)->membername)
#define TYPE_FROM_MEMBER(type, memberptr, membername) \
    ((type *)((intptr_t)(memberptr) - OFFSETOF(type, membername)))

/* Initialize a master list head */
void list_initialize(struct list_item *master_list_head);

/* Are there items after the head item? */
bool list_is_empty(struct list_item *head_item);

/* Does the item belong to a list? */
bool list_is_item_listed(struct list_item *item);

/* Is the item a member in a particular list? */
bool list_is_member(struct list_item *master_list_head,
                    struct list_item *item);

/* Remove an item from a list - no head item needed */
void list_remove_item(struct list_item *item);

/* Add a list item after the base item */
void list_add_item(struct list_item *head_item,
                   struct list_item *item);

/* Clear list items after the head item */
void list_clear_all(struct list_item *head_item);

/* Enumerate all items after the head item - passing each item in turn
 * to the callback as well as the data value. The current item may be
 * safely removed. Items added after the current position will be enumated
 * but not ones added before it. The callback may return false to stop
 * the enumeration. */
typedef bool (*list_enum_callback_t)(struct list_item *item, intptr_t data);

void list_enum_items(struct list_item *head_item,
                     list_enum_callback_t callback,
                     intptr_t data);

#endif /* MPEG_LINKEDLIST_H */

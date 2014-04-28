/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
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
#include <stddef.h>
#include "linked_list.h"

/**
 * Finds the node previous to "find"
 */
static struct ll_node * ll_search_prev(struct ll_head *list,
                                       struct ll_node *find)
{
    struct ll_node *prev = NULL;
    struct ll_node *node = list->head;

    while (node && node != find)
    {
        prev = node;
        node = node->next;
    }

    return prev;
}


/** (L)inked (L)ist **/

/**
 * Initializes the singly-linked list
 */
void ll_init(struct ll_head *list)
{
    list->head = NULL;
    list->tail = NULL;
}

/**
 * Adds a node to s singly-linked list using "insert next"
 */
void ll_insert_next(struct ll_head *list, struct ll_node *prevnode,
                    struct ll_node *node)
{
    if (prevnode == NULL) /* At head */

    {
        node->next = list->head;

        if (list->head == NULL)
            list->tail = node;

        list->head = node;
    }
    else                  /* Anywhere else */
    {
        struct ll_node *next = prevnode->next;

        node->next = next;

        if (next == NULL)
            list->tail = node;

        prevnode->next = node;
    }
}

/**
 * Adds a node to a singly-linked list using "insert last"
 */
void ll_insert_last(struct ll_head *list, struct ll_node *node)
{
    node->next = NULL;

    if (list->head == NULL)
        list->head = node; /* Unoccupied */
    else
        list->tail->next = node;

    list->tail = node;
}

/**
 * Removes the node after "node"; if there is none, nothing is changed
 */
void ll_remove_next(struct ll_head *list, struct ll_node *node)
{
    struct ll_node *next = list->head;

    if (next == NULL)
        return;

    if (node == NULL)
    {
        /* Remove first item */
        next = next->next;
        list->head = next;

        if (!next)
            list->tail = NULL;
    }
    else
    {
        next = node->next;

        if (next)
        {
            next = next->next;
            node->next = next;

            if (!next)
                list->tail = node;
        }
    }
}

/**
 * Removes the node from the list
 */
void ll_remove(struct ll_head *list, struct ll_node *node)
{
    ll_remove_next(list, ll_search_prev(list, node));
}


/** (L)inked (L)ist (D)ouble **/

/**
 * Initializes the doubly-linked list
 */
void lld_init(struct lld_head *list)
{
    list->head = NULL;
}

/** (L)inked (L)ist (D)ouble (C)ircular **/

/**
 * Adds an item to a doubly-linked circular list using "insert first"
 */
void lldc_insert_first(struct lld_head *list, struct lld_node *node)
{
    struct lld_node *head = list->head;

    if (head == NULL) /* Unoccupied? */
    {
        node->prev = node;
        node->next = node;
        list->head = node;
    }
    else
    {
        node->prev = head->prev;
        node->next = head;
        head->prev->next = node;
        head->prev = node;
        list->head = node;
    }
}

/**
 * Adds an item to a doubly-linked circular list using "insert last"
 */
void lldc_insert_last(struct lld_head *list, struct lld_node *node)
{
    struct lld_node *head = list->head;

    if (head == NULL) /* Unoccupied? */
    {
        node->prev = node;
        node->next = node;
        list->head = node;
    }
    else
    {
        node->prev = head->prev;
        node->next = head;
        head->prev->next = node;
        head->prev = node;
    }
}

/**
 * Removes an item from a doubly-linked circular list
 */
void lldc_remove(struct lld_head *list, struct lld_node *node)
{
    struct lld_node *next = node->next;

    if (node == list->head)
    {
        if (node == next) /* Last one? */
        {
            list->head = NULL;
            return;
        }

        list->head = next;
    }

    struct lld_node *prev = node->prev;

    /* Fix links to jump over the removed entry */
    next->prev = prev;
    prev->next = next;
}

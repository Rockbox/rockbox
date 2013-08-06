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
 * Finds the node previous to 'find'
 *
 * returns: NULL if 'find' is the first node
 *          last node if the 'find' isn't found in the list
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
    if (prevnode == NULL)   /* at head */
    {
        node->next = list->head;

        if (list->head == NULL)
            list->tail = node;

        list->head = node;
    }
    else                    /* anywhere else */
    {
        struct ll_node *next = prevnode->next;

        node->next = next;

        if (next == NULL)
            list->tail = node;

        prevnode->next = node;
    }
}

/**
 * Adds a node to a singly-linked list using "insert first"
 */
void ll_insert_first(struct ll_head *list, struct ll_node *node)
{
    ll_insert_next(list, NULL, node);
}

/**
 * Adds a node to a singly-linked list using "insert last"
 */
void ll_insert_last(struct ll_head *list, struct ll_node *node)
{
    node->next = NULL;

    if (list->head == NULL)
        list->head = node;  /* unoccupied */
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

    if (node == NULL)       /* remove first node */
    {
        next = next->next;
        list->head = next;

        if (!next)
            list->tail = NULL;
    }
    else                    /* remove next node */
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
 * Removes the first item in the list
 */
void ll_remove_first(struct ll_head *list)
{
    ll_remove_next(list, NULL);
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

/* inserts an node into the list; does not affect list->head */
static inline struct lld_node * lldc_insert(struct lld_head *list,
                                            struct lld_node *node)
{
    struct lld_node *head = list->head;

    if (head == NULL)       /* unoccupied? */
    {
        node->prev = node;  /* point node to itself */
        node->next = node;
    }
    else                    /* insert after last item */
    {
        node->prev = head->prev;
        node->next = head;
        head->prev->next = node;
        head->prev = node;
        
    }

    return head;
}

/**
 * Adds a node to a doubly-linked circular list using "insert first"
 */
void lldc_insert_first(struct lld_head *list, struct lld_node *node)
{
    lldc_insert(list, node);
    list->head = node;      /* rotate list head to node */
}

/**
 * Adds a node to a doubly-linked circular list using "insert last"
 */
void lldc_insert_last(struct lld_head *list, struct lld_node *node)
{
    if (!lldc_insert(list, node))
        list->head = node;  /* list was empty; node is first */
}

/**
 * Removes a node from a doubly-linked circular list
 */
void lldc_remove(struct lld_head *list, struct lld_node *node)
{
    struct lld_node *next = node->next;

    if (node == list->head) /* is first node? */
    {
        if (node == next)   /* last one? */
        {
            list->head = NULL;
            return;
        }

        list->head = next;  /* head becomes next node */
    }

    /* link node's prev and next together */
    struct lld_node *prev = node->prev;
    next->prev = prev;
    prev->next = next;
}

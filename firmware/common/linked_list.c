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


/** (L)inked (L)ist **/

/**
 * Helper to find the node previous to 'find'
 *
 * returns: NULL if 'find' is the first node
 *          last node if the 'find' isn't found in the list
 */
static struct ll_node * ll_search_prev(struct ll_head *list,
                                       struct ll_node *find)
{
    struct ll_node *prev = NULL;
    struct ll_node *node = list->head;

    while (node != NULL && node != find)
    {
        prev = node;
        node = node->next;
    }

    return prev;
}

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
void ll_insert_next(struct ll_head *list, struct ll_node *node,
                    struct ll_node *newnode)
{
    struct ll_node **nodep = node != NULL ? &node->next : &list->head;
    struct ll_node *next = *nodep;

    newnode->next = next;
    *nodep = newnode;

    if (next == NULL)
        list->tail = newnode;
}

/**
 * Adds a node to a singly-linked list using "insert first"
 */
void ll_insert_first(struct ll_head *list, struct ll_node *node)
{
    struct ll_node *head = list->head;

    node->next = head;
    list->head = node;

    if (head == NULL)
        list->tail = node;
}

/**
 * Adds a node to a singly-linked list using "insert last"
 */
void ll_insert_last(struct ll_head *list, struct ll_node *node)
{
    struct ll_node *tail = list->tail;

    node->next = NULL;
    list->tail = node;

    if (tail == NULL)
        list->head = node;
    else
        tail->next = node;
}

/**
 * Removes the node after "node"; if there is none, nothing is changed
 */
void ll_remove_next(struct ll_head *list, struct ll_node *node)
{
    struct ll_node **nodep = node != NULL ? &node->next : &list->head;
    struct ll_node *next = *nodep;

    if (next != NULL)
    {
        next = next->next;
        *nodep = next;

        if (next == NULL)
           list->tail = node;
    }
}

/**
 * Removes the first node in the list
 */
void ll_remove_first(struct ll_head *list)
{
    struct ll_node *node = list->head->next;

    list->head = node;

    if (node == NULL)
        list->tail = NULL;
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
    list->tail = NULL;

    /* tail could be stored in first item's prev pointer but this simplifies
       the routines and maintains the non-circularity */
}

/**
 * Adds a node to a doubly-linked list using "insert first"
 */
void lld_insert_first(struct lld_head *list, struct lld_node *node)
{
    struct lld_node *head = list->head;

    list->head = node;

    if (head == NULL)
        list->tail = node;
    else
        head->prev = node;

    node->next = head;
    node->prev = NULL;
}

/**
 * Adds a node to a doubly-linked list using "insert last"
 */
void lld_insert_last(struct lld_head *list, struct lld_node *node)
{
    struct lld_node *tail = list->tail;

    list->tail = node;

    if (tail == NULL)
        list->head = node;
    else
        tail->next = node;

    node->next = NULL;
    node->prev = tail;
}

/**
 * Removes a node from a doubly-linked list
 */
void lld_remove(struct lld_head *list, struct lld_node *node)
{
    struct lld_node *next = node->next;
    struct lld_node *prev = node->prev;

    if (node == list->head)
        list->head = next;

    if (node == list->tail)
        list->tail = prev;

    if (prev != NULL)
        prev->next = next;

    if (next != NULL)
        next->prev = prev;
}


/** (L)inked (L)ist (D)ouble (C)ircular **/

/**
 * Helper that inserts a node into a doubly-link circular list; does not
 * affect list->head, just returns its state
 */
static inline struct lldc_node * lldc_insert(struct lldc_head *list,
                                             struct lldc_node *node)
{
    struct lldc_node *head = list->head;

    if (head == NULL)
    {
        node->prev = node;
        node->next = node;
    }
    else
    {
        node->prev = head->prev;
        node->next = head;
        head->prev->next = node;
        head->prev = node;
    }

    return head;
}

/**
 * Initializes the doubly-linked circular list
 */
void lldc_init(struct lldc_head *list)
{
    list->head = NULL;
}

/**
 * Adds a node to a doubly-linked circular list using "insert first"
 */
void lldc_insert_first(struct lldc_head *list, struct lldc_node *node)
{
    lldc_insert(list, node);
    list->head = node;
}

/**
 * Adds a node to a doubly-linked circular list using "insert last"
 */
void lldc_insert_last(struct lldc_head *list, struct lldc_node *node)
{
    if (lldc_insert(list, node) == NULL)
        list->head = node;
}

/**
 * Removes a node from a doubly-linked circular list
 */
void lldc_remove(struct lldc_head *list, struct lldc_node *node)
{
    struct lldc_node *next = node->next;

    if (node == list->head)
    {
        if (node == next)
        {
            list->head = NULL;
            return;
        }

        list->head = next;
    }

    struct lldc_node *prev = node->prev;
    next->prev = prev;
    prev->next = next;
}

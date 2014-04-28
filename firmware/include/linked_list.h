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
#ifndef LINKED_LIST_H
#define LINKED_LIST_H

/***
 ** NOTE: node field order chosen so that one type can alias the other for
 **       forward list traveral by the same code
 */

/* Singly-linked list structures */
struct ll_node
{
    struct ll_node *next; /* Next list item */
};

struct ll_head
{
    struct ll_node *head; /* First list item */
    struct ll_node *tail; /* Last list item (to make insert_last O(1)) */
};

/* Singly-linked list functions */

/* Format:
 * head->XX->YY->ZZ->NULL
 *       XX  YY  ZZ
 * tail----------^
 */

void ll_init(struct ll_head *list);
void ll_insert_next(struct ll_head *list, struct ll_node *prevnode,
                    struct ll_node *node);
void ll_insert_last(struct ll_head *list, struct ll_node *node);
void ll_remove_next(struct ll_head *list, struct ll_node *node);
void ll_remove(struct ll_head *list, struct ll_node *node);

/* Doubly-linked list structures */
struct lld_node
{
    struct lld_node *next; /* Next list item (*) */
    struct lld_node *prev; /* Previous list item */
};

struct lld_head
{
    struct lld_node *head; /* First list item */
};

/* Doubly-linked list functions */
void lld_init(struct lld_head *list);

/* Doubly-linked circular list functions */

/* Format:
 *       +--<---<---<---+
 * head->+->XX->YY->ZZ->+
 *       +<-XX<-YY<-ZZ<-+
 *       +--->--->--->--+
 */
void lldc_insert_first(struct lld_head *list, struct lld_node *node);
void lldc_insert_last(struct lld_head *list, struct lld_node *node);
void lldc_remove(struct lld_head *list, struct lld_node *node);

#endif /* LINKED_LIST_H */

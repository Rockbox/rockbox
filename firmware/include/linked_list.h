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
 ** NOTES:
 ** Node field order chosen so that one type can alias the other for forward
 ** list traveral by the same code.
 **
 ** Structures are separate, even if logically equivalent, to perform compile-
 ** time type checking.
 **
 */


/* (L)inked (L)ist
 *
 * Format:
 *
 *       XX->YY->ZZ->0
 * head--^       ^
 * tail----------+
 */
struct ll_head
{
    struct ll_node *head; /* First list item */
    struct ll_node *tail; /* Last list item (to make insert_last O(1)) */
};

struct ll_node
{
    struct ll_node *next; /* Next list item */
};

void ll_init(struct ll_head *list);
void ll_insert_next(struct ll_head *list, struct ll_node *node,
                    struct ll_node *newnode);
void ll_insert_last(struct ll_head *list, struct ll_node *node);
void ll_remove_next(struct ll_head *list, struct ll_node *node);
void ll_remove(struct ll_head *list, struct ll_node *node);
void ll_insert_first(struct ll_head *list, struct ll_node *node);
void ll_remove_first(struct ll_head *list);


/* (L)inked (L)ist (D)double
 *
 * Format:
 *
 *      0<-XX<->YY<->ZZ->0
 * head----^         ^
 * tail--------------+
 */
struct lld_head
{
    struct lld_node *head; /* First list item */
    struct lld_node *tail; /* Last list item (to make insert_last O(1)) */
};

struct lld_node
{
    struct lld_node *next; /* Next list item */
    struct lld_node *prev; /* Previous list item */
};

void lld_init(struct lld_head *list);
void lld_insert_first(struct lld_head *list, struct lld_node *node);
void lld_insert_last(struct lld_head *list, struct lld_node *node);
void lld_remove(struct lld_head *list, struct lld_node *node);


/* (L)inked (L)ist (D)ouble (C)ircular
 *
 * Format:
 *        +----------------+
 *        |                |
 *        +->XX<->YY<->ZZ<-+
 * head------^
 */
struct lldc_head
{
    struct lldc_node *head; /* First list item */
};

struct lldc_node
{
    struct lldc_node *next; /* Next list item */
    struct lldc_node *prev; /* Previous list item */
};

void lldc_init(struct lldc_head *list);
void lldc_insert_first(struct lldc_head *list, struct lldc_node *node);
void lldc_insert_last(struct lldc_head *list, struct lldc_node *node);
void lldc_remove(struct lldc_head *list, struct lldc_node *node);

#endif /* LINKED_LIST_H */

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 by Michael Sevakis
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
#ifndef _SEARCHTREE_H_
#define _SEARCHTREE_H_

#include <stdbool.h>

/* node's in the binary tree */
struct searchtree_node
{
    unsigned long          key;      /* node's stored sorted key */
    struct searchtree_node *left;    /* pointer to left child */
    struct searchtree_node *right;   /* pointer to right child */
    struct searchtree_node *up;      /* pointer to parent */
    struct searchtree_node *prev;    /* previous node in order */
    struct searchtree_node *next;    /* next node in order */
    unsigned int           priority; /* random heap priority */
};

/* the meta root of the data structure */
struct searchtree
{
    struct searchtree_node *root;    /* root node of search tree */
    struct searchtree_node *min;     /* node with minimum key */
    struct searchtree_node *max;     /* node with maximum key */
    unsigned int           random;   /* priority random seed */
};

/* initialize a new searchtree data structure */
void searchtree_init(struct searchtree *tree);

/* search for a key in the tree and return its node, or NULL if it doesn't
 * exist; provide a buffer in upp to receive parent, or candidate parent if
 * key wasn't found */
struct searchtree_node * searchtree_search(struct searchtree *tree,
                                           unsigned long key,
                                           struct searchtree_node **upp);

/* insert a node set to a given key under the parent */
void searchtree_insert(struct searchtree *tree,
                       struct searchtree_node *node,
                       unsigned long key,
                       struct searchtree_node *up);

/* remove a node, adjusting the candidate parent if necessary. upp may be
 * NULL if this is a simple removal and there is no insert to follow */
void searchtree_remove(struct searchtree *tree,
                       struct searchtree_node *node,
                       unsigned long key,
                       struct searchtree_node **upp);

/* check and adjust range - use before calling searchtree_range_start()
   to speed access and reject pointless searches */
static inline bool searchtree_check_range(struct searchtree *tree,
                                          unsigned long *key_minp,
                                          unsigned long *key_maxp)
{
    if (UNLIKELY(!tree->root)) {
        return false;
    }

    /* clip to real range available */
    if (*key_minp < tree->min->key) {
        *key_minp = tree->min->key;
    }

    if (*key_maxp > tree->max->key) {
        *key_maxp = tree->max->key;
    }

    return *key_maxp >= *key_minp;
}

/* return node with smallest key within range key_min ... key_max, else
 * NULL if none */
struct searchtree_node * searchtree_range_start(struct searchtree *tree,
                                                unsigned long key_min,
                                                unsigned long key_max);

/* return the node with the minimum key in the order, NULL if empty */
static inline struct searchtree_node *
    searchtree_min(const struct searchtree *tree)
{
    return tree->min;
}

/* return the node with the maximum key in the order, NULL if empty */
static inline struct searchtree_node *
    searchtree_max(const struct searchtree *tree)
{
    return tree->max;
}

/* return the node's key */
static inline unsigned long
    searchtree_key_of(const struct searchtree_node *node)
{
    return node->key;
}

/* return the node's inorder predecessor, if any, else NULL */
static inline struct searchtree_node *
    searchtree_predecessor_of(const struct searchtree_node *node)
{
    return (struct searchtree_node *)node->prev;
}

/* return the node's inorder successor, if any, else NULL */
static inline struct searchtree_node *
    searchtree_successor_of(const struct searchtree_node *node)
{
    return (struct searchtree_node *)node->next;
}

#endif /* _SEARCHTREE_H_ */

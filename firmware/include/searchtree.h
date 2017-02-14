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

struct searchtree_node
{
    unsigned long          key;       /* node's key */
    union {
    intptr_t               updir;     /* parent + L/R sentinel (as node) */
    uint32_t               random;    /* random seed (as root pseudonode) */
    };
    struct searchtree_node *child[2]; /* left and right children */
    struct searchtree_node *list[2];  /* in-order prev/next */
    uint32_t               priority;  /* heap priority */
};

struct searchtree
{
    struct searchtree_node root;      /* root pseudo node */
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

static inline struct searchtree_node *
    searchtree_root_node(const struct searchtree *tree)
{
    return tree->root.child[0];
}

static inline bool searchtree_check_range(struct searchtree *tree,
                                          unsigned long *key_minp,
                                          unsigned long *key_maxp)
{
    if (UNLIKELY(!searchtree_root_node(tree))) {
        return false;
    }

    /* clip to real range available */
    if (*key_minp < tree->root.list[1]->key) {
        *key_minp = tree->root.list[1]->key;
    }

    if (*key_maxp > tree->root.list[0]->key) {
        *key_maxp = tree->root.list[0]->key;
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
    struct searchtree_node *min = tree->root.list[1];
    return min != &tree->root ? min : NULL;
}

/* return the node with the maximum key in the order, NULL if empty */
static inline struct searchtree_node *
    searchtree_max(const struct searchtree *tree)
{
    struct searchtree_node *max = tree->root.list[0];
    return max != &tree->root ? max : NULL;
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
    struct searchtree_node *prev = node->list[0];
    return prev->priority ? prev : NULL;
}

/* return the node's inorder successor, if any, else NULL */
static inline struct searchtree_node *
    searchtree_successor_of(const struct searchtree_node *node)
{
    struct searchtree_node *next = node->list[1];
    return next->priority ? next : NULL;
}

#endif /* _SEARCHTREE_H_ */

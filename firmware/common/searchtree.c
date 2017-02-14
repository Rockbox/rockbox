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
#include "config.h"
#include "system.h"
#include "searchtree.h"
#include <stdint.h>

/***
 * Binary search tree implemented as a treap
 *
 * Optimized for frequent key replacement, range queries and inorder
 * traversal.
 *
 * Min and max of key range is kept efficiently since an inorder linked list
 * is maintained. Range queries need only one binary search for the first
 * element in the range. Finsh the range query by following the list until
 * an element greater than or equal to the max is seen or the list ends.
 *
 * Looking up a key returns a parent node hint that may be adjusted when
 * removing a key. Passing this hint when inserting a new key when a key
 * is not found only requires a single binary search for a replace. Standard
 * search tree removal protocol is used to have an O(1) remove operation, not
 * rotating the tree of the root to be removed until it has only one child as
 * is the usual implementation for this structure.
 *
 * Nodes and keys are paired for life; i.e. keys are never copied to a
 * different node to facilitate removal of an internal node. This is essential
 * for the intended application.
 */

/* Left or right child flag is kept in low bit of parent pointer */
#define UPDIR_UP(updir) \
    ((struct searchtree_node *)((updir) & ~(intptr_t)1))

#define UP_UPDIR(up, dir) \
    ((intptr_t)(up) | (dir))

#define NODE_UP(node) \
    UPDIR_UP((node)->updir)

#define NODE_DIR(node) \
    ((node)->updir & (intptr_t)1)

/* get the next random priority */
static FORCE_INLINE uint32_t get_random_priority(struct searchtree *tree)
{
    /* cheapo random scammed from DSP white noise generator */
    return (tree->root.random = tree->root.random*0x0019660dU
                + 0x3c6ef35fU) | 1;
}

/* initialize a new searchtree data structure */
void searchtree_init(struct searchtree *tree)
{
    /* pick up garbage data off the stack */
    unsigned int seed = seed;
    seed = *(volatile unsigned int *)&seed;
    tree->root.key      = 0;           /* ensure all nodes are right children */
    tree->root.random   = seed;
    tree->root.child[0] = NULL;        /* never used */
    tree->root.child[1] = NULL;        /* first real root node goes here */
    tree->root.list[0]  = &tree->root; /* in-order ring is circular */
    tree->root.list[1]  = &tree->root; /* list[0] is max, and list[1] is min */
    tree->root.priority = 0;           /* mark as list end/top (as min heap) */
}

/* search for a key in the tree and return its node, or NULL if it doesn't
 * exist */
struct searchtree_node * searchtree_search(struct searchtree *tree,
                                           unsigned long key,
                                           struct searchtree_node **upp)
{
    /* if there are duplicate keys, this returns the first match it finds,
       which could be any one of them */
    struct searchtree_node *up = &tree->root;
    struct searchtree_node *node = searchtree_root_node(tree);

    while (node && node->key != key) {
        up = node;
        node = key < node->key ? node->child[0] : node->child[1];
    }

    if (upp) {
        /* if not found, this is the parent it should have */
        *upp = node ? NULL : up;
    }

    return node;
}

/* insert a node with the given key - return existing node if found or else
   the node paramater */
void searchtree_insert(struct searchtree *tree,
                       struct searchtree_node *node,
                       unsigned long key,
                       struct searchtree_node *up)
{
    if (UNLIKELY(!up)) {
        /* no parent given; find place for forced insert; duplicates go right
           in order to enforce sorting stability */
        up = &tree->root;

        struct searchtree_node *next = searchtree_root_node(tree);

        while (next) {
            up = next;
            next = key < next->key ? next->child[0] : next->child[1];
        }
    }

    int dir = key >= up->key;
    int rid = 1 - dir;

    up->child[dir] = node;

    /* add to place in inorder list */
    node->list[dir] = up->list[dir];
    node->list[rid] = up;
    node->list[dir]->list[rid] = node;
    up->list[dir] = node;

    /* fill node fields */
    node->key      = key;
    node->updir    = UP_UPDIR(up, dir);
    node->child[0] = NULL;
    node->child[1] = NULL;
    node->priority = get_random_priority(tree);

    /* rotate parent subtree if priority is inverted */
    while (up->priority > node->priority) {
        /* rol =>          <= ror
         *    u              u
         *    |              |
         *    a              N
         *   / \            / \
         *  x   N   <==>   a   z
         *     / \        / \
         *    y   z      x   y
         */
        struct searchtree_node *y = node->child[rid];

        up->child[dir]   = y;
        node->updir      = up->updir;
        up->updir        = UP_UPDIR(node, rid);
        node->child[rid] = up;
        if (y) {
            y->updir = UP_UPDIR(up, dir);
        }

        up = NODE_UP(node);
        up->child[NODE_DIR(node)] = node;

        dir = NODE_DIR(node);
        rid = 1 - dir;
    }
}

/* remove a node from the tree */
void searchtree_remove(struct searchtree *tree,
                       struct searchtree_node *node,
                       unsigned long key,
                       struct searchtree_node **upp)
{
    /* remove node from inorder list */
    node->list[0]->list[1] = node->list[1];
    node->list[1]->list[0] = node->list[0];

    struct searchtree_node *up = NODE_UP(node);
    struct searchtree_node *dn;

    if (node->child[0] && node->child[1]) {
        /* two-child case: randomly pick predecessor or successor from
           priority bottom bit and move it into the place of the node
           being removed */
        int movedir = node->priority >> 31;
        int moverid = 1 - movedir;

        struct searchtree_node *move = node->list[movedir];
        struct searchtree_node *moveup = NODE_UP(move);

        move->child[moverid] = node->child[moverid];
        node->child[moverid]->updir = UP_UPDIR(move, moverid);

        if (moveup != node) {
            if (move->child[movedir]) {
                move->child[movedir]->updir = UP_UPDIR(moveup, moverid);
            }

            moveup->child[moverid] = move->child[movedir];

            move->child[movedir] = node->child[movedir];
            node->child[movedir]->updir = UP_UPDIR(move, movedir);
        }

        if (UNLIKELY(upp && move == *upp))  {
            /* suggested parent needs adjusting */
            if (movedir ? (key < move->key) : (key >= move->key)) {
                *upp = node->list[moverid];
            }
            else if (moveup != node) {
                *upp = moveup;
            }
        }

        move->priority = node->priority;
        dn = move;
    }
    else {
        /* one or no child case; just remove and link its child with its
           parent */
        dn = node->child[0] ?: node->child[1];

        if (UNLIKELY(upp && node == *upp)) {
            /* suggested parent needs adjusting */
            *upp = dn ? (key < node->key ? node->list[1] : node->list[0]) : up;
        }
    }

    up->child[NODE_DIR(node)] = dn;

    if (dn) {
        dn->updir = node->updir;
    }

    (void)tree;
}

/* return node with smallest key within range key_min ... key_max, else
 * NULL if none */
struct searchtree_node * searchtree_range_start(struct searchtree *tree,
                                                unsigned long key_min,
                                                unsigned long key_max)
{
    /* find the smallest key >= key_min; returns the least-recently-inserted
       of a set of matching duplicates */
    struct searchtree_node *next = searchtree_root_node(tree);
    struct searchtree_node *node = NULL;

    while (next) {
        unsigned long key = next->key;

        if (key >= key_min) {
            if (!node || key <= node->key) {
                node = next;
            }

            next = next->child[0];
        }
        else {
            next = next->child[1];
        }
    }

    if (LIKELY(node && node->key <= key_max)) {
        return node;
    }

    /* not in range afterall */
    return NULL;
}

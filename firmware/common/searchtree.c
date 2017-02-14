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

/***
 * Binary search tree implemented as a treap
 *
 * Optimized for frequent key replacement, range queries and inorder
 * traversal.
 *
 * Duplicate keys are not currently supported.
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

/* get the next random priority */
static inline unsigned int get_random_priority(struct searchtree *tree)
{
    /* treapo random scammed from DSP white noise generator */
    return (tree->random = tree->random*0x0019660dU + 0x3c6ef35fU);
}

/* rotate a tree left or right which preserves the key order and moves the
 * root down one level */
static void rotate_tree(struct searchtree_node **dnp,
                        struct searchtree_node *node,
                        bool ror)
{
    /*  ror:                  rol:
     *      u         u         u             u
     *      |         |         |             |
     *      N         a         N             a
     *     / \       / \       / \           / \
     *    a   z ==> x   N     x   a   ==>   N   z
     *   / \           / \       / \       / \
     *  x   y         y   z     y   z     x   y
     */
    struct searchtree_node *up = node->up;
    if (LIKELY(up)) {
        dnp = node->key < up->key ? &up->left : &up->right;
    }

    if (ror) {
        struct searchtree_node *left = node->left;
        *dnp = left;
        left->up    = up;
        node->up    = left;
        node->left  = left->right;
        left->right = node;
        if (node->left) {
            node->left->up = node;
        }
    }
    else { /* rol */
        struct searchtree_node *right = node->right;
        *dnp = right;
        right->up   = up;
        node->up    = right;
        node->right = right->left;
        right->left = node;
        if (node->right) {
            node->right->up = node;
        }
    }
}

/* initialize a new treap data structure */
void searchtree_init(struct searchtree *tree)
{
    /* pick up garbage data off the stack */
    unsigned int seed = *(volatile unsigned int *)&seed;
    tree->root   = NULL;
    tree->min    = NULL;
    tree->max    = NULL;
    tree->random = seed;
}

/* search for a key in the treap and return its node, or NULL if it doesn't
 * exist; provide a buffer in upp to receive parent, or candidate parent if
 * key wasn't found */
struct searchtree_node * searchtree_search(struct searchtree *tree,
                                           unsigned long key,
                                           struct searchtree_node **upp)
{
    struct searchtree_node *up = NULL;
    struct searchtree_node *node = tree->root;

    while (node && node->key != key) {
        up = node;
        node = key < node->key ? node->left : node->right;
    }

    if (upp) {
        /* if not found, this is the parent it should have */
        *upp = up;
    }

    return node;
}

/* insert a node set to a given key under the parent */
void searchtree_insert(struct searchtree *tree,
                       struct searchtree_node *node,
                       unsigned long key,
                       struct searchtree_node *up)
{
    struct searchtree_node **dnp;

    if (LIKELY(up)) {
        /* adjust min and/or max if needed */
        if (key < tree->min->key) {
            tree->min = node;
        }
        else if (key > tree->max->key) {
            tree->max = node;
        }

        /* add to place in inorder list */
        if (key < up->key) {
            /* left child - is up's inorder predecessor */
            node->prev = up->prev;
            node->next = up;
            if (up->prev) {
                up->prev->next = node;
            }
            up->prev = node;
            dnp = &up->left;
        }
        else {
            /* right child - is up's inorder successor */
            node->next = up->next;
            node->prev = up;
            if (up->next) {
                up->next->prev = node;
            }
            up->next = node;
            dnp = &up->right;
        }
    }
    else {
        /* first node */
        dnp = &tree->root;
        tree->min = tree->max = node;
        node->prev = node->next = NULL;
    }

    *dnp = node;
    node->key      = key;
    node->left     = NULL;
    node->right    = NULL;
    node->up       = up;
    node->priority = get_random_priority(tree);

    /* rotate parent subtree if priority is inverted */
    while (node->up && node->up->priority < node->priority) {
        rotate_tree(&tree->root, node->up, key < node->up->key);
    }
}

/* remove a node, adjusting the candidate parent if necessary. upp may be
 * NULL if this is a simple removal and there is no insert to follow */
void searchtree_remove(struct searchtree *tree,
                       struct searchtree_node *node,
                       unsigned long key,
                       struct searchtree_node **upp)
{
    if (UNLIKELY(!tree->root)) {
        return;
    }

    /* fix up min and max if needed */
    if (node == tree->min) {
        tree->min = node->next;
    }

    if (node == tree->max) {
        tree->max = node->prev;
    }

    /* remove node from inorder list (node's own list pointers are not
       altered) */
    if (node->next) {
        node->next->prev = node->prev;
    }

    if (node->prev) {
        node->prev->next = node->next;
    }

    struct searchtree_node *up = node->up;
    struct searchtree_node **dnp = &tree->root, *dn;

    if (LIKELY(up)) {
        dnp = node->key < up->key ? &up->left : &up->right;
    }

    if (node->left && node->right) {
        /* two-child case: randomly pick predecessor or successor from
           priority bottom bit and move it into the place of the node
           being removed */
        struct searchtree_node *move;
        struct searchtree_node **nodexp, **nodeyp,
                               **movexp, **moveyp, **moveupdnp;
        if (node->priority & 1) {
            /* successor */
            move = node->next;

            if (UNLIKELY(upp && move == *upp))  {
                /* suggested parent needs adjusting */
                if (key < move->key) {
                    *upp = node->prev;
                }
                else if (move->up != node) {
                    *upp = move->up;
                }
            }

            nodexp    = &node->left;
            nodeyp    = &node->right;
            movexp    = &move->left;
            moveyp    = &move->right;
            moveupdnp = &move->up->left;

        }
        else {
            /* predecessor */
            move = node->prev;

            if (UNLIKELY(upp && move == *upp))  {
                /* suggested parent needs adjusting */
                if (key > move->key) {
                    *upp = node->next;
                }
                else if (move->up != node) {
                    *upp = move->up;
                }
            }

            nodexp    = &node->right;
            nodeyp    = &node->left;
            movexp    = &move->right;
            moveyp    = &move->left;
            moveupdnp = &move->up->right;
        }

        *movexp = *nodexp;
        (*movexp)->up = move;

        if (move->up != node) {
            if (*moveyp) {
                (*moveyp)->up = move->up;
            }

            *moveupdnp    = *moveyp;
            *moveyp       = *nodeyp;
            (*nodeyp)->up = move;
        }

        move->up = up;
        move->priority = node->priority;
        dn = move;
    }
    else {
        /* one or no child case; just remove and link its child with its
           parent */
        if (UNLIKELY(upp && node == *upp)) {
            /* suggested parent needs adjusting */
            if (key < node->key) {
                *upp = node->right ? node->next : node->up;
            }
            else {
                *upp = node->left ? node->prev : node->up;
            }
        }

        if ((dn = node->left ?: node->right)) {
            dn->up = up;
        }
    }

    *dnp = dn;
}

/* return node with smallest key within range key_min ... key_max, else
 * NULL if none */
struct searchtree_node * searchtree_range_start(struct searchtree *tree,
                                                unsigned long key_min,
                                                unsigned long key_max)
{
    /* find the smallest key >= key_min */
    struct searchtree_node *next = tree->root;
    struct searchtree_node *node = NULL;

    while (next) {
        unsigned long key = next->key;

        if (key >= key_min && (!node || key < node->key)) {
            node = next;
            if (key == key_min) {
                break;
            }
        }

        next = key_min < key ? next->left : next->right;
    }

    if (LIKELY(node && node->key <= key_max)) {
        /* not in range afterall */
        return node;
    }

    return NULL;
}

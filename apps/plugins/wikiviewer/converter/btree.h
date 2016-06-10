/*B-tree from lkml before 23.07.2006, licensed under GPLv2.
 *  btree.c Copyright (C) 2006 Vishal Patil (vishpat AT gmail DOT com) Copyright
 *(C) 2006 Frederik M.J.V. (comparistion and write functions) TREE_EDITABLE
 *should work, but isn't tested.
 */
#ifndef _BTREE_H_
#define _BTREE_H_

/* Platform dependent headers */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <strings.h>

#define DEBUG
#define mem_alloc malloc
#define mem_free free
#define bcopy bcopy
#define print printf

typedef struct {
    void * key;
    void * val;
} bt_key_val;

typedef struct bt_node {
    struct bt_node * next;      /* Pointer used for linked list  */
    bool leaf;          /* Used to indicate whether leaf or not */
    unsigned int nr_active;     /* Number of active keys */
    unsigned int level;     /* Level in the B-Tree */
    bt_key_val ** key_vals;     /* Array of keys and values */
    struct bt_node ** children; /* Array of pointers to child nodes */
} bt_node;

typedef struct {
    unsigned int order;         /* B-Tree order */
    bt_node * root;             /* Root of the B-Tree */
    unsigned int (*key_size)(void * key);    /* Return the key size */
    unsigned int (*data_size)(void * data);  /* Return the data size */
    char (*compare)(void *key1,void *key2); /* compare keys return(ret):
                                               1<2:ret<0 1=2:ret=0 1>2:ret>0 */
    void (*node_write)(FILE *file,void *tree,bt_node *node,uint32_t *chldptrs);
    unsigned short (*node_write_len)(void *tree,bt_node *node);
    void (*print_key)(void * key);      /* Print the key */
} btree;

extern btree * btree_create(unsigned int order);
extern int btree_insert_key(btree * btree, bt_key_val * key_val);
extern int btree_delete_key(btree * btree,bt_node * subtree,void * key);
extern bt_key_val * btree_search(btree * btree,  void * key);
extern void btree_destroy(btree * btree);
extern void * btree_get_max_key(btree * btree);
extern void * btree_get_min_key(btree * btree);
uint32_t btree_write(FILE *file,btree *btree,bt_node *node);
#ifdef DEBUG
extern void print_subtree(btree * btree,bt_node * node);
#endif


#endif

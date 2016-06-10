/*B-tree from lkml before 23.07.2006, licensed under GPLv2.
 *  btree.c Copyright (C) 2006 Vishal Patil (vishpat AT gmail DOT com) Copyright
 *(C) 2006 Frederik M.J.V. (comparistion and write functions) TREE_EDITABLE
 *should work, but isn't tested.
 */

/*LE write function from rockbox, scramble.c Copyright (C) 2002 by Bjørn
   Stenberg GPL*/
#include "btree.h"

typedef enum {left = -1,right = 1} position_t;

typedef struct {
    bt_node * node;
    unsigned int index;
} node_pos;

static void print_single_node(btree *btree, bt_node * node);
static bt_node * allocate_btree_node (unsigned int order);
static int free_btree_node (bt_node * node);

static node_pos get_btree_node(btree * btree,void * key);

/**
 *    Used to create a btree with just the root node
 *    @param order The order of the B-tree
 *    @return The an empty B-tree
 */
btree * btree_create(unsigned int order)
{
    btree * btree;
    btree = mem_alloc(sizeof(*btree));
    btree->order = order;
    btree->root = allocate_btree_node(order);
    btree->root->leaf = true;
    btree->root->nr_active = 0;
    btree->root->next = NULL;
    btree->root->level = 0;
    return btree;
}

/**
 *       Function used to allocate memory for the btree node
 *       @param order Order of the B-Tree
 *    @param leaf boolean set true for a leaf node
 *       @return The allocated B-tree node
 */
static bt_node * allocate_btree_node (unsigned int order)
{
    bt_node * node;

    /*Allocate memory for the node */
    node = (bt_node *)mem_alloc(sizeof(bt_node));

    /* Initialize the number of active nodes */
    node->nr_active = 0;

    /* Initialize the keys */
    node->key_vals = (bt_key_val **)mem_alloc(2*order*sizeof(bt_key_val*) - 1);

    /* Initialize the child pointers */
    node->children = (bt_node **)mem_alloc(2*order*sizeof(bt_node*));

    /* Use to determine whether it is a leaf */
    node->leaf = true;

    /* Use to determine the level in the tree */
    node->level = 0;

    /* Initialize the linked list pointer to NULL */
    node->next = NULL;

    return node;
}

/**
 *       Function used to free the memory allocated to the b-tree
 *       @param node The node to be freed
 *       @param order Order of the B-Tree
 *       @return The allocated B-tree node
 */
static int free_btree_node (bt_node * node)
{
    mem_free(node->children);
    mem_free(node->key_vals);
    mem_free(node);

    return 0;
}

/**
 *    Used to split the child node and adjust the parent so that it has two
 *children
 *    @param parent Parent Node
 *    @param index Index of the child node
 *    @param child  Full child node
 *
 */
static void btree_split_child(btree * btree, bt_node * parent,
                              unsigned int index,
                              bt_node * child)
{
    unsigned int i = 0;
    unsigned int order = btree->order;

    bt_node * new_child = allocate_btree_node(btree->order);
    new_child->leaf = child->leaf;
    new_child->level = child->level;
    new_child->nr_active = btree->order - 1;

    /* Copy the higher order keys to the new child    */
    for(i=0; i<order - 1; i++)
    {
        new_child->key_vals[i] = child->key_vals[i + order];
        if(!child->leaf)
            new_child->children[i] =
                child->children[i + order];
    }

    /*  Copy the last child pointer */
    if(!child->leaf)
        new_child->children[i] =
            child->children[i + order];

    child->nr_active = order - 1;

    for(i = parent->nr_active + 1; i > index + 1; i--)
    {
        parent->children[i] = parent->children[i - 1];
    }
    parent->children[index + 1] = new_child;

    for(i = parent->nr_active; i > index; i--)
    {
        parent->key_vals[i] = parent->key_vals[i - 1];
    }

    parent->key_vals[index] = child->key_vals[order - 1];
    parent->nr_active++;
}

/**
 *    Used to insert a key in the non-full node
 *    @param btree The btree
 *    @param node The node to which the key will be added
 *    @param the key value pair
 *    @return void
 */

static void btree_insert_nonfull (btree * btree, bt_node * parent_node,
                                  bt_key_val * key_val)
{
    int i;
    bt_node * child;
    bt_node * node = parent_node;

insert:    i = node->nr_active - 1;
    if(node->leaf)
    {
        while(i >= 0 && btree->compare(key_val->key,node->key_vals[i]->key)<0)
        {
            node->key_vals[i + 1] = node->key_vals[i];
            i--;
        }
        node->key_vals[i + 1] = key_val;
        node->nr_active++;
    }
    else
    {
        while (i >= 0 && btree->compare(key_val->key,node->key_vals[i]->key)<0)
        {
            i--;
        }
        i++;
        child = node->children[i];

        if(child->nr_active == 2*btree->order - 1)
        {
            btree_split_child(btree,node,i,child);
            if(btree->compare(key_val->key,node->key_vals[i]->key)>0)
                i++;
        }

        node = node->children[i];
        goto insert;
    }
}

/**
 *       Function used to insert node into a B-Tree
 *       @param root Root of the B-Tree
 *       @param node The node to be inserted
 *       @param compare Function used to compare the two nodes of the tree
 *       @return success or failure
 */
int btree_insert_key(btree * btree, bt_key_val * key_val)
{
    bt_node * rnode;

    rnode = btree->root;
    if(rnode->nr_active == (2*btree->order - 1))
    {
        bt_node * new_root;
        new_root = allocate_btree_node(btree->order);
        new_root->level = btree->root->level + 1;
        btree->root = new_root;
        new_root->leaf = false;
        new_root->nr_active = 0;
        new_root->children[0]  = rnode;
        btree_split_child(btree,new_root,0,rnode);
        btree_insert_nonfull(btree,new_root,key_val);
    }
    else
        btree_insert_nonfull(btree,rnode,key_val);

    return 0;
}

/**
 *    Function used to get the node containing the given key
 *    @param btree The btree to be searched
 *    @param key The the key to be searched
 *    @return The node and position of the key within the node
 */
node_pos get_btree_node(btree * btree,void * key)
{
    node_pos kp;
    kp.node=NULL;
    kp.index=0;
    bt_node * node;
    unsigned int i = 0;
    node = btree->root;

    for (;; i = 0)
    {
        /* Fix the index of the key greater than or equal to the key that we
           would like to search */

        while (i < node->nr_active && btree->compare(key, node->key_vals[i]->key)>0 )
            i++;

        /* If we find such key return the key-value pair */
        if(i < node->nr_active && btree->compare(key,node->key_vals[i]->key)==0)
        {
            kp.node = node;
            kp.index = i;
            return kp;
        }

        /* If the node is leaf and if we did not find the key
           return NULL */
        if(node->leaf)
            return kp;

        /* To got a child node */
        node = node->children[i];
    }
    return kp;
}

/**
 *       Used to destory btree
 *       @param btree The B-tree
 *       @return none
 */
void btree_destroy(btree * btree)
{
    unsigned int i;
    unsigned int current_level;

    bt_node * head, * tail, * node;
    bt_node * child, * del_node;

    node = btree->root;
    current_level = node->level;
    head = node;
    tail = node;

    while(true)
    {
        if(head == NULL)
            break;

        if (head->level < current_level)
            current_level = head->level;

        if(head->leaf == false)
            for(i = 0; i < head->nr_active + 1; i++)
            {
                child = head->children[i];
                tail->next = child;
                tail = child;
                child->next = NULL;
            }

        del_node = head;
        head = head->next;
        free_btree_node(del_node);
    }
}

/**
 *       Function used to search a node in a B-Tree
 *       @param btree The B-tree to be searched
 *       @param key Key of the node to be search
 *       @return The key-value pair
 */
bt_key_val * btree_search(btree * btree,void * key)
{
    bt_key_val * key_val = NULL;
    node_pos kp = get_btree_node(btree,key);

    if(kp.node)
        key_val = kp.node->key_vals[kp.index];

    return key_val;
}

#ifdef DEBUG
#include <stdint.h>
/**
 *    Used to print the keys of the bt_node
 *    @param node The node whose keys are to be printed
 *    @return none
 */

static void print_single_node(btree *btree, bt_node * node)
{
    unsigned int i = 0;

    print(" { ");
    while(i < node->nr_active)
    {
        print("(%d)%s(%d) ",btree->key_size(node->key_vals[i]->key),
              (char*)(node->key_vals[i]->key+sizeof(int16_t)),node->level);
        i++;
    }
    print("} (0x%x,%d) ", (unsigned int)node,node->leaf);
}

/**
 *       Function used to print the B-tree
 *       @param root Root of the B-Tree
 *       @param print_key Function used to print the key value
 *       @return none
 */

void print_subtree(btree *btree,bt_node * node)
{
    unsigned int i;
    unsigned int current_level;

    bt_node * head, * tail;
    bt_node * child;

    current_level = node->level;
    head = node;
    tail = node;

    while(true)
    {
        if(head == NULL)
            break;

        if (head->level < current_level)
        {
            current_level = head->level;
            print("\n");
        }

        print_single_node(btree,head);

        if(head->leaf == false)
            for(i = 0; i < head->nr_active + 1; i++)
            {
                child = head->children[i];
                tail->next = child;
                tail = child;
                child->next = NULL;
            }

        head = head->next;
    }
    print("\n");
}

uint32_t btree_write(FILE *file,btree *btree,bt_node *node)
{
    long ndestrtfo,tmpfo;
    unsigned short i;
    ndestrtfo=ftell(file);
    if(node==0)
    {
        printf("NDNL\n");
        return 0;
    }

    uint32_t chldptrs[node->nr_active+1];
    fseek(file,ndestrtfo+btree->node_write_len(btree,node),SEEK_SET);
    if(node->leaf==false)
        for(i=0; i < node->nr_active+1; i++)
        {
            chldptrs[i]=btree_write(file,btree,node->children[i]);
        }

    tmpfo=ftell(file);
    fseek(file,ndestrtfo,SEEK_SET);
    btree->node_write(file,btree,node,chldptrs);
    fseek(file,tmpfo,SEEK_SET);
    return (uint32_t)ndestrtfo;
}

#endif

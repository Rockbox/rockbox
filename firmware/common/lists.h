/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef LISTS_INCLUDED
#define LISTS_INCLUDED
/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
*               DESCRIPTION                                         *
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
           
  Functions for handling of a double linked list (code in lists.c):
  
  list_init                     - Initiate a LIST structure
  list_insert_before            - Insert a node in a list before another node
  list_insert_after             - Insert a node in a list after another node
  list_extract                  - Extract a node from a list
  list_extract_first            - Extract a node from the beginning of a list
  list_extract_last             - Extract a node from the end of a list
  list_add_first                - Add a node at the beginning of a list
  list_add_last                 - Add a node at the end of a list
  list_last_in_list             - Check if a node is last in the list
  list_first_in_list            - Check if a node is first in the list
  list_empty                    - Check if a list is empty
  list_get_first                - Returns the first node in the list
  list_get_last                 - Returns the last node in the list
  list_get_next                 - Returns the next node
  list_get_prev                 - Returns the previous node
   
 General about the list functions:
 
 This package contains two structs and a set of functions implementing a
 double linked list. When using a list with these, add nodes with
 list_add_first or list_add_last and extract nodes with list_extract_first or list_extract_last.
 
 A FIFO queue can be implemented using list_add_last to insert
 nodes at the end and list_extract_first to extract nodes at the beginning
 of the list.

 When using the list-functions you need one instance of the type LIST
 (contains the start pointer etc), You also need to call InitList once.
 When calling InitList you decide if the list shall have a max size,
 and if so how big it will be.
 
 NOTE! when using list_insert_before, list_insert_after and
 list_extract the num_nodes field in the LIST struct will not be updated.
 
 To use the list-functions you define a struct with LIST_NODE as one member.
 When a LIST_NODE is extracted from the list the address of the LIST_NODE 
 is returned. If the LIST_NODE field is not the first field in your struct,
 you must subtract the returned address with the size of the field that is 
 before LIST_NODE in your struct to get the start address of your struct.
    
 For example: Consider you want to use the list-functions to implement a FIFO
 queue for the test_list_sig signal, using the OSE operating system.
 
 You may declare your signal struct something like:
 
    typedef struct
    {
       SIGSELECT sig_no;
       LIST_NODE list_node;
       .
       .
    } TEST_LIST_STRUCT;
    
 In your code you may have:
 
    LIST test_list;
    TEST_LIST_STRUCT *list_sig;
    LIST_NODE *ln;
        
    list_init_list(&test_list, MAX_NODES_IN_LIST);
    
    for(;;)
    {
        sig = receive(all);
        switch(sig->sig_no)
        {
           case TEST_ADD:
              if (list_add_first(&test_list, &sig->list_node))
              {
                  * The list was full *
                  * Handle the returned sig *
              }
              
           case TEST_REM:
                if (ln = list_extract_last(&test_list))
                {
                  list_sig = (TEST_LIST_STRUCT *)((char)ln - sizeof(SIGSELECT));
                  * Continue ... *
                }
                else
                {
                   * handle empty list *
                   
*/

/*--------------------------------------------------------------------
   List handling definitions
----------------------------------------------------------------------*/

/* Used as input parameter to list_init_list */
#define NO_SIZE_CHECK 0

#ifndef NULL
#define NULL ((void *)0)
#endif


/***********************************************************************
** Type definitions
**
** The LIST_NODE, and LIST types are used by the LIST class in common drv
************************************************************************/


typedef struct list_node
{
                                /*** NOTE! The next and prev fields 
                                     are never NULL within a LIST.
                                     Use list_first_in_list and 
                                     list_last_in_list to check if a node 
                                     is first or last in the list     ***/
  struct list_node *next;            /* Successor node                  */
  struct list_node *prev;            /* Predessor node                  */
} LIST_NODE;

typedef struct
{
  /* The three list node fields are 
     used for internal represenation 
     of the list.                               */
  LIST_NODE *first;             /* First node in list                   */
  LIST_NODE *last_prev;         /* Always NULL                          */
  LIST_NODE *last;              /* Last node in list                    */
  int num_nodes;                /* Number of nodes in list              */
  int max_num_nodes;            /* Max number of nodes in list          */
} LIST;


/******************************************************************************
**
** FUNCTION:    list_init_list
**
** DESCRIPTION: Initiate a LIST structure.
**
** INPUT:       The LIST to be initiated
**      
**              Max nr of elements in the list, or NO_SIZE_CHECK if 
**              list may be of any size
*******************************************************************************
*/
void list_init_list(LIST *list,               /* The list to initiate */
		    int max_num_nodes);       /* Maximum number of nodes */

/******************************************************************************
**
** FUNCTION:    list_insert_before
**
** DESCRIPTION: Insert a LIST_NODE before another LIST_NODE in a LIST.
**
** INPUT:       The reference LIST_NODE
**
**              The LIST_NODE to be inserted
**
** OBSERVE:     When using list_insert_before, list_insert_after
**              and list_extract the nume_nodes field in the LIST struct
**              will not be updated.
******************************************************************************
*/
void list_insert_before(LIST_NODE *ref,    /* The reference node */
			LIST_NODE *ln);    /* The node to insert */

/*****************************************************************************
**
** FUNCTION:    list_insert_after
**
** DESCRIPTION: Insert a LIST_NODE after another LIST_NODE in a LIST.
**
** INPUT:       The reference LIST_NODE
**
**              The LIST_NODE to be inserted
**
** OBSERVE:     When using list_insert_before, list_insert_after
**              and list_extract the num_nodes field in the LIST struct
**              will not be updated.
*******************************************************************************
*/
void list_insert_after(LIST_NODE *ref,     /* The reference node */
                     LIST_NODE *ln);       /* The node to insert */

/******************************************************************************
**
** FUNCTION:    list_extract
**
** DESCRIPTION: Extract a LIST_NODE from a LIST.
**
** INPUT:       The LIST_NODE to be removed.
**
** RETURN:      The same LIST_NODE pointer that was passed as a parameter
**
** OBSERVE:     When using list_insert_before, list_insert_after
**              and list_extract the num_nodes field in the LIST struct
**              will not be updated.
*******************************************************************************
*/
LIST_NODE *list_extract(LIST_NODE *ln);    /* The node to extract */

/******************************************************************************
**
** FUNCTION:    list_add_first
**
** DESCRIPTION: Add a LIST_NODE at the beginning of a LIST.
**
** INPUT:       The LIST
**
**              The LIST_NODE 
**
** RETURN:      TRUE if OK
**              FALSE if list is full
*******************************************************************************
*/
int list_add_first(LIST *list,       /* The list to add to */
		   LIST_NODE *ln);   /* The node to add */

/******************************************************************************
**
** FUNCTION:    list_extract_first
**
** DESCRIPTION: Extract a LIST_NODE from the beginning of a LIST.
**
** INPUT:       The LIST from where a node is to be removed.
**
** RETURN:      The extracted LIST_NODE or NULL if the list is empty
*******************************************************************************
*/
LIST_NODE *list_extract_first(LIST *list);      /* The list to extract from */

/******************************************************************************
**
** FUNCTION:    list_add_last
**
** DESCRIPTION: Add a LIST_NODE at the end of a LIST.
**
** INPUT:       The LIST
**
**              The LIST_NODE 
**
** RETURN:      TRUE if OK
**              FALSE if list is full
*******************************************************************************
*/
int list_add_last(LIST *list,        /* The list to add to */
		  LIST_NODE *ln);    /* The node to add  */
/******************************************************************************
**
** FUNCTION:    list_extract_last
**
** DESCRIPTION: Extract a LIST_NODE from the end of a LIST.
**
** INPUT:       The LIST from where a node is to be removed.
**
** RETURN:      The extracted LIST_NODE or NULL if the list is empty
*******************************************************************************
*/
LIST_NODE *list_extract_last(LIST *list);   /* The list to extract from */

/******************************************************************************
**
** FUNCTION:    list_last_in_list
**
** DESCRIPTION: Check if a LIST_NODE is at the end of the list
**
** INPUT:       The LIST_NODE to check
**
** RETURN:      TRUE if LIST_NODE is last in the list, else FALSE.
*******************************************************************************
*/
int list_last_in_list(LIST_NODE *ln);       /* The node to check */

/******************************************************************************
**
** FUNCTION:    list_first_in_list
**
** DESCRIPTION: Check if a LIST_NODE is first in the list
**
** INPUT:       The LIST_NODE to check
**
** RETURN:      TRUE if LIST_NODE is first in the list, else FALSE.
*******************************************************************************
*/
int list_first_in_list(LIST_NODE *ln);  /* The node to check */

/******************************************************************************
**
** FUNCTION:    list_list_empty
**
** DESCRIPTION: Check if a LIST is empty
**
** INPUT:       The LIST to check
**
** RETURN:      TRUE if LIST is empty, else FALSE.
**
*******************************************************************************
*/ 
int list_empty(LIST *list);     /* The list to check */

/******************************************************************************
** FUNCTION:    list_get_first
**
** DESCRIPTION: Return a LIST_NODE from the beginning of a LIST.
**
** RETURN:      The first LIST_NODE or NULL if the list is empty
******************************************************************************/
LIST_NODE *list_get_first(LIST *lh);  /* The list to read from */

/******************************************************************************
** FUNCTION:    list_get_last
**
** DESCRIPTION: Return a LIST_NODE from the end of a LIST.
**
** RETURN:      The last LIST_NODE or NULL if the list is empty
******************************************************************************/
LIST_NODE *list_get_last(LIST *lh);   /* The list to read from */

/******************************************************************************
** FUNCTION:    list_get_next
**
** DESCRIPTION: Return the LIST_NODE following the specified one.
**
** RETURN:      Next LIST_NODE or NULL if the list ends here
*******************************************************************************/
LIST_NODE *list_get_next(LIST_NODE *ln); /* The list node to get next from */

/******************************************************************************
** FUNCTION:    list_get_prev
**
** DESCRIPTION: Return the LIST_NODE preceding the specified one.
**
** RETURN:      Previous LIST_NODE or NULL if the list ends here
*******************************************************************************/
LIST_NODE *list_get_prev(LIST_NODE *ln); /* The list node to get previous from */

#endif /* LISTS_INCLUDED */

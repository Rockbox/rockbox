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
/*
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Description:
  
  This file contains functions implementing a double linked list. 
  The functions uses the types LIST and LISTNODE.
  There is a trick with the three nodes in LIST. By placing the 
  tail_pred field in middle, makes it possible to avoid special code
  to handle nodes in the beginning or end of the list.
  
  The 'head' and 'tail' field in LIST are never NULL, even if the
  list is empty.
  
  The 'succ' and 'pred' field in a LIST_NODE that is in the list,
  are never NULL. The 'pred' field for the first LIST_NODE points
  to the 'head' field in LIST. The 'succ' node for the last LIST_NODE
  points the the tail_pred field in LIST.
  
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
#include "lists.h"

/****************************************************************************
** FUNCTION:    list_init
**
** DESCRIPTION: Initiate a LIST structure. 
**              If max_num_nodes is set to NO_SIZE_CHECK there 
**              will be no check of the length of the list.
**
** RETURN:      Nothing
******************************************************************************/
void list_init(LIST *list,         /* The list to initiate */
	       int max_num_nodes)  /* Maximum number of nodes */
{
   list->first = (LIST_NODE *)&list->last_prev;
   list->last_prev = NULL;
   list->last = (LIST_NODE *)&list->first;
   list->num_nodes = 0;
   list->max_num_nodes = max_num_nodes;
}

/****************************************************************************
** FUNCTION:    list_insert_before
**
** DESCRIPTION: Insert a LIST_NODE in a list before another node in a LIST.
**
** RETURN:      Nothing
******************************************************************************/
void list_insert_before(LIST_NODE *ref,    /* The reference node */
			     LIST_NODE *ln)     /* The node to insert */
{
   ln->next = ref;               /* ref is after us */
   ln->prev = ref->prev;         /* ref's prev is before us */
   ref->prev = ln;               /* we are before ref */
   ln->prev->next = ln;          /* we are after ref's prev */
}

/****************************************************************************
** FUNCTION:    list_insert_after
**
** DESCRIPTION: Insert a LIST_NODE in a list after another node in a LIST.
**
** RETURN:      Nothing
*****************************************************************************/
void list_insert_after(LIST_NODE *ref,     /* The reference node */
			    LIST_NODE *ln)      /* The node to insert */
{
   ln->prev = ref;               /* ref is before us */
   ln->next = ref->next;         /* ref's next is after us */
   ref->next = ln;               /* we are after ref */
   ln->next->prev = ln;          /* we are before ref's next */
}

/****************************************************************************
** FUNCTION:    list_extract_node
**
** DESCRIPTION: Extract a LIST_NODE from a list.
**
** RETURN:      The same LIST_NODE pointer that was passed as a parameter.
*****************************************************************************/
LIST_NODE *list_extract_node(LIST_NODE *ln)     /* The node to extract */
{
  ln->prev->next = ln->next;      /* Our prev's next points to our next */
  ln->next->prev = ln->prev;      /* Our next's prev points to our prev */
  return ln;
}

/******************************************************************************
** FUNCTION:    list_add_first
**
** DESCRIPTION: Add a LIST_NODE at the beginning of a LIST.
**
** RETURN:      1 if OK
**              0 if list was full
******************************************************************************/
int list_add_first(LIST *list,             /* The list to add to */
                 LIST_NODE *ln)         /* The node to add */
{
   if (NO_SIZE_CHECK != list->max_num_nodes)
   {
      if(list->num_nodes >= list->max_num_nodes)  /* List full? */
      {
	 return 0;
      }
   }
   list_insert_after((LIST_NODE *)list, ln);
   list->num_nodes++;            /* Increment node counter */
   return 1;
}

/******************************************************************************
** FUNCTION:    list_extract_first
**
** DESCRIPTION: Extract a LIST_NODE from the beginning of a LIST.
**
** RETURN:      The extracted LIST_NODE or NULL if the list is empty
******************************************************************************/
LIST_NODE *list_extract_first(LIST *list)       /* The list to extract from */
{
   LIST_NODE *ln;
  
   if(list_empty(list))             /* Return NULL if the list is empty */
   {
      return NULL;
   }
  
   ln = list_extract_node((LIST_NODE *)list->first);  /* Get first node */
  
   list->num_nodes--;                    /* Decrement node counter */
  
   return ln;
}

/******************************************************************************
** FUNCTION:    list_add_last
**
** DESCRIPTION: Add a LIST_NODE at the end of a LIST.
**
** RETURN:      NULL if OK
**              0 if list was full
******************************************************************************/
int list_add_last(LIST *list,              /* The list to add to */
                LIST_NODE *ln)          /* The node to add */
{
   if (NO_SIZE_CHECK != list->max_num_nodes)
   {
      if(list->num_nodes >= list->max_num_nodes)  /* List full? */
      {
	 return 0;      
      }
   }
   list_insert_before((LIST_NODE *)&list->last_prev, ln);
   list->num_nodes++;                    /* Increment node counter */
   return 1;
}

/******************************************************************************
** FUNCTION:    list_extract_last
**
** DESCRIPTION: Extract a LIST_NODE from the end of a LIST.
**
** RETURN:      The extracted LIST_NODE or NULL if the list is empty
******************************************************************************/
LIST_NODE *list_extract_last(LIST *list)        /* The list to extract from */
{
   LIST_NODE *ln;
  
   if(list_empty(list))             /* Return NULL if the list is empty */
   {
      return NULL;
   }
  
   ln = list_extract_node((LIST_NODE *)list->last);
  
   list->num_nodes--;    /* Decrement node counter */
  
   return ln;           /* Is NULL if the list is empty */
}

/******************************************************************************
** FUNCTION:    list_last_in_list
**
** DESCRIPTION: Check if a LIST_NODE is last in the list
**
** RETURN:      1 if last in list
******************************************************************************/
int list_last_in_list(LIST_NODE *ln)    /* The node to check */
{
   return ln->next->next == NULL;
}

/*****************************************************************************
** FUNCTION:    list_first_in_list
**
** DESCRIPTION: Check if a LIST_NODE is first in the list
**
** RETURN:      1 if first in list
******************************************************************************/
int list_first_in_list(LIST_NODE *ln)   /* The node to check */
{
   return ln->prev->prev == NULL;
}

/******************************************************************************
** FUNCTION:    list_empty
**
** DESCRIPTION: Check if a LIST is empty
**
** RETURN:      1 if list is empty
******************************************************************************/
int list_empty(LIST *list)         /* The list to check */
{
   return list->first == (LIST_NODE *)&list->last_prev;
}

/******************************************************************************
** FUNCTION:    list_get_first
**
** DESCRIPTION: Return a LIST_NODE from the beginning of a LIST.
**
** RETURN:      The first LIST_NODE or NULL if the list is empty
******************************************************************************/
LIST_NODE *list_get_first(LIST *lh)   /* The list to read from */
{
   if(list_empty(lh))       /* Return NULL if the list is empty */
   {
      return NULL;
   }
  
   return lh->first;     /* Get first node */
}

/******************************************************************************
** FUNCTION:    list_get_last
**
** DESCRIPTION: Return a LIST_NODE from the end of a LIST.
**
** RETURN:      The last LIST_NODE or NULL if the list is empty
******************************************************************************/
LIST_NODE *list_get_last(LIST *lh)    /* The list to read from */
{
   if(list_empty(lh))               /* Return NULL if the list is empty */
   {
      return NULL;
   }
  
   return lh->last;
}

/******************************************************************************
** FUNCTION:    list_get_next
**
** DESCRIPTION: Return the LIST_NODE following the specified one.
**
** RETURN:      Next LIST_NODE or NULL if the list ends here
*******************************************************************************/
LIST_NODE *list_get_next(LIST_NODE *ln) /* The list node to get next from */
{
   if(list_last_in_list(ln))       /* Return NULL if this is the end of list */
   {
      return NULL;
   }
  
   return ln->next;
}

/******************************************************************************
** FUNCTION:    list_get_prev
**
** DESCRIPTION: Return the LIST_NODE preceding the specified one.
**
** RETURN:      Previous LIST_NODE or NULL if the list ends here
*******************************************************************************/
LIST_NODE *list_get_prev(LIST_NODE *ln) /* The list node to get previous from */
{
   if(list_first_in_list(ln))            /* Return NULL if this is the start of list */
   {
      return NULL;
   }
  
   return ln->prev;
}

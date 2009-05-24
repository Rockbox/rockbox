/*****************************************************************************
 *
 * Size-sorted list/tree functions.
 *
 * Author:  Daniel Stenberg
 * Date:    March 7, 1997
 * Version: 2.0
 * Email:   Daniel.Stenberg@sth.frontec.se
 *
 *
 * v2.0
 * - Added SPLAY TREE functionality.
 * 
 * Adds and removes CHUNKS from a list or tree.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#define SPLAY /* we use the splay version as that is much faster */

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef SPLAY /* these routines are for the non-splay version */

struct ChunkInfo {
  struct ChunkInfo *larger;
  struct ChunkInfo *smaller;
  size_t size;
};

/* the CHUNK list anchor */
struct ChunkInfo *chunkHead=NULL;

/***********************************************************************

  findchunkbysize()

  Find the chunk that is smaller than the input size. Returns
  NULL if none is.

  **********************************************************************/

static struct ChunkInfo *findchunkbysize(size_t size)
{
  struct ChunkInfo *test = chunkHead;
  struct ChunkInfo *smaller = NULL;
  while(test && (test->size < size)) {
    smaller = test;
    test = test->larger;
  }
  return smaller;
}

/***********************************************************************
  
  remove_chunksize()

  Remove the chunk from the size-sorted list.
  ***********************************************************************/

void remove_chunksize(void *data)
{
  struct ChunkInfo *chunk = (struct ChunkInfo *)data;
  if(chunk->smaller)
    chunk->smaller->larger = chunk->larger;
  else {
    /* if this has no smaller, this is the head */
    chunkHead = chunk->larger; /* new head */
  }
  if(chunk->larger)
    chunk->larger->smaller = chunk->smaller;
}

void insert_bysize(char *data, size_t size)
{
  struct ChunkInfo *newchunk = (struct ChunkInfo *)data;
  struct ChunkInfo *chunk = findchunkbysize ( size );

  newchunk->size = size;

  if(chunk) {
    /* 'chunk' is smaller than size, append the new chunk ahead of this */
    newchunk->smaller = chunk;
    newchunk->larger = chunk->larger;
    if(chunk->larger)
      chunk->larger->smaller = newchunk;
    chunk->larger = newchunk;
  }
  else {
    /* smallest CHUNK around, append first in the list */
    newchunk->larger = chunkHead;
    newchunk->smaller = NULL;

    if(chunkHead)
      chunkHead->smaller = newchunk;
    chunkHead = newchunk;
  }
}

char *obtainbysize( size_t size)
{
  struct ChunkInfo *chunk = findchunkbysize( size );

  if(!chunk) {
    if(size <= (chunkHead->size))
      /* there is no smaller CHUNK, use the first one (if we fit within that)
       */
      chunk = chunkHead;
  }
  else
    /* we're on the last CHUNK that is smaller than requested, step onto
       the bigger one */
    chunk = chunk->larger;

  if(chunk) {
    remove_chunksize( chunk ); /* unlink size-wise */
    return (char *)chunk;
  }
  else
    return NULL;
}

void print_sizes(void)
{
  struct ChunkInfo *chunk = chunkHead;
  printf("List of CHUNKS (in size order):\n");
#if 1
  while(chunk) {
    printf("  START %p END %p SIZE %d\n",
	   chunk, (char *)chunk+chunk->size, chunk->size);
    chunk = chunk->larger;
  }
#endif
  printf("End of CHUNKS:\n");
}

#else /* Here follows all routines dealing with the SPLAY TREES */

typedef struct tree_node Tree;
struct tree_node {
  Tree *smaller; /* smaller node */
  Tree *larger;  /* larger node */
  Tree *same;    /* points to a node with identical key */
  int key;       /* the "sort" key */
};

Tree *chunkHead = NULL; /* the root */

#define compare(i,j) ((i)-(j))

/* Set this to a key value that will *NEVER* appear otherwise */
#define KEY_NOTUSED -1

/*
 * Splay using the key i (which may or may not be in the tree.) The starting
 * root is t. Weight fields are maintained.
 */
Tree * splay (int i, Tree *t)
{
  Tree N, *l, *r, *y;
  int comp;
    
  if (t == NULL)
    return t;
  N.smaller = N.larger = NULL;
  l = r = &N;

  for (;;) {
    comp = compare(i, t->key);
    if (comp < 0) {
      if (t->smaller == NULL)
	break;
      if (compare(i, t->smaller->key) < 0) {
	y = t->smaller;                           /* rotate smaller */
	t->smaller = y->larger;
	y->larger = t;

	t = y;
	if (t->smaller == NULL)
	  break;
      }
      r->smaller = t;                               /* link smaller */
      r = t;
      t = t->smaller;
    }
    else if (comp > 0) {
      if (t->larger == NULL)
	break;
      if (compare(i, t->larger->key) > 0) {
	y = t->larger;                          /* rotate larger */
	t->larger = y->smaller;
	y->smaller = t;
	t = y;
	if (t->larger == NULL)
	  break;
      }
      l->larger = t;                              /* link larger */
      l = t;
      t = t->larger;
    }
    else {
      break;
    }
  }

  l->larger = r->smaller = NULL;

  l->larger = t->smaller;                                /* assemble */
  r->smaller = t->larger;
  t->smaller = N.larger;
  t->larger = N.smaller;

  return t;
}

/* Insert key i into the tree t.  Return a pointer to the resulting tree or
   NULL if something went wrong. */
Tree *insert(int i, Tree *t, Tree *new)
{
  if (new == NULL) {
    return t;
  }

  if (t != NULL) {
    t = splay(i,t);
    if (compare(i, t->key)==0) {
      /* it already exists one of this size */

      new->same = t;
      new->key = i;
      new->smaller = t->smaller;
      new->larger = t->larger;

      t->smaller = new;
      t->key = KEY_NOTUSED;

      return new; /* new root node */
    }
  }

  if (t == NULL) {
    new->smaller = new->larger = NULL;
  }
  else if (compare(i, t->key) < 0) {
    new->smaller = t->smaller;
    new->larger = t;
    t->smaller = NULL;
  }
  else {
    new->larger = t->larger;
    new->smaller = t;
    t->larger = NULL;
  }
  new->key = i;

  new->same = NULL; /* no identical node (yet) */

  return new;
}

/* Finds and deletes the best-fit node from the tree. Return a pointer to the
   resulting tree.  best-fit means the smallest node that fits the requested
   size. */
Tree *removebestfit(int i, Tree *t, Tree **removed)
{
  Tree *x;

  if (t==NULL)
    return NULL;
  t = splay(i,t);
  if(compare(i, t->key) > 0) {
    /* too small node, try the larger chain */
    if(t->larger)
      t=splay(t->larger->key, t);
    else {
      /* fail */
      *removed = NULL;
      return t;
    }
  }

  if (compare(i, t->key) <= 0) {               /* found it */

    /* FIRST! Check if there is a list with identical sizes */
    x = t->same;
    if(x) {
      /* there is, pick one from the list */

      /* 'x' is the new root node */

      x->key = t->key;
      x->larger = t->larger;
      x->smaller = t->smaller;
      *removed = t;
      return x; /* new root */
    }

    if (t->smaller == NULL) {
      x = t->larger;
    }
    else {
      x = splay(i, t->smaller);
      x->larger = t->larger;
    }
    *removed = t;

    return x;
  }
  else {
    *removed = NULL; /* no match */
    return t;        /* It wasn't there */
  }
}


/* Deletes the node we point out from the tree if it's there. Return a pointer
   to the resulting tree.  */
Tree *removebyaddr(Tree *t, Tree *remove)
{
  Tree *x;

  if (!t || !remove)
    return NULL;

  if(KEY_NOTUSED == remove->key) {
    /* just unlink ourselves nice and quickly: */
    remove->smaller->same = remove->same;
    if(remove->same)
      remove->same->smaller = remove->smaller;
    /* voila, we're done! */
    return t;
  }

  t = splay(remove->key,t);

  /* Check if there is a list with identical sizes */

  x = t->same;
  if(x) {
    /* 'x' is the new root node */

    x->key = t->key;
    x->larger = t->larger;
    x->smaller = t->smaller;
      
    return x; /* new root */
  }

  /* Remove the actualy root node: */

  if (t->smaller == NULL) {
    x = t->larger;
  }
  else {
    x = splay(remove->key, t->smaller);
    x->larger = t->larger;
  }

  return x;
}

int printtree(Tree * t, int d, char output)
{
  int distance=0;
  Tree *node;
  int i;
  if (t == NULL)
    return 0;
  distance += printtree(t->larger, d+1, output);
  for (i=0; i<d; i++)
    if(output)
      printf("  ");

  if(output) {
    printf("%d[%d]", t->key, i);
  }
  
  for(node = t->same; node; node = node->same) {
    distance += i; /* this has the same "virtual" distance */

    if(output)
      printf(" [+]");
  }
  if(output)
    puts("");

  distance += i;
  distance += printtree(t->smaller, d+1, output);
  return distance;
}

/* Here follow the look-alike interface so that the tree-function names are
   the same as the list-ones to enable easy interchange */

void remove_chunksize(void *data)
{
  chunkHead = removebyaddr(chunkHead, data);
}

void insert_bysize(char *data, size_t size)
{
  chunkHead = insert(size, chunkHead, (Tree *)data);
}

char *obtainbysize( size_t size)
{
  Tree *receive;
  chunkHead = removebestfit(size, chunkHead, &receive);
  return (char *)receive;
}

void print_sizes(void)
{
  printtree(chunkHead, 0, 1);
}

#endif

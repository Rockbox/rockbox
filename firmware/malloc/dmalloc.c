/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/*****************************************************************************
 *
 * Dynamic small-blocks Memory Allocation
 *
 * Author:  Daniel Stenberg <daniel@haxx.se>
 *
 * Read THOUGHTS for theories and details on the implementation.
 *
 *****************************************************************************/

#include <stdio.h>
#include <string.h> /* memcpy */

#ifdef DEBUG_MALLOC
#include <stdarg.h>
#endif

#ifdef PSOS
#include <psos.h>
#define SEMAPHORE /* the PSOS routines use semaphore protection */
#else

#endif

#define BMALLOC /* we use our own big-malloc system */

#ifdef BMALLOC
#include "bmalloc.h"
#endif

/* Each TOP takes care of a chain of BLOCKS */
struct MemTop {
  struct MemBlock *chain; /* pointer to the BLOCK chain */
  long nfree;             /* total number of free FRAGMENTS in the chain */
  short nmax;             /* total number of FRAGMENTS in this kind of BLOCK */
  size_t fragsize;        /* the size of each FRAGMENT */

#ifdef SEMAPHORE /* if we're protecting the list with SEMAPHORES */
  long semaphore_id;      /* semaphore used to lock this particular list */
#endif

};

/* Each BLOCK takes care of an amount of FRAGMENTS */
struct MemBlock {
    struct MemTop *top;     /* our TOP struct */
    struct MemBlock *next;  /* next BLOCK */
    struct MemBlock *prev;  /* prev BLOCK */
  
    struct MemFrag *first;  /* the first free FRAGMENT in this block */

    short nfree;            /* number of free FRAGMENTS in this BLOCK */
};

/* This is the data kept in all _free_ FRAGMENTS */
struct MemFrag {
    struct MemFrag *next;   /* next free FRAGMENT */
    struct MemFrag *prev;   /* prev free FRAGMENT */
};

/* This is the data kept in all _allocated_ FRAGMENTS and BLOCKS. We add this
   to the allocation size first thing in the ALLOC function to make room for
   this smoothly. */

struct MemInfo {
    void *block;
    /* which BLOCK is our father, if BLOCK_BIT is set it means this is a
       stand-alone, large allocation and then the rest of the bits should be
       treated as the size of the block */
#define BLOCK_BIT 1
};

/* ---------------------------------------------------------------------- */
/*                                 Defines                                */
/* ---------------------------------------------------------------------- */

#ifdef DEBUG_VERBOSE
#define MEMINCR(addr,x) memchange(addr, x)
#define MEMDECR(addr,x) memchange(addr,-(x))
#else
#define MEMINCR(a,x)
#define MEMDECR(a,x)
#endif

/* The low level functions used to get memory from the OS and to return memory
   to the OS, we may also define a stub that does the actual allocation and
   free, these are the defined function names used in the dmalloc system
   anyway: */
#ifdef PSOS

#ifdef DEBUG_MALLOC
#define DMEM_OSALLOCMEM(size,pointer,type) pointer=(type)dbgmalloc(size)
#define DMEM_OSFREEMEM(x) dbgfree(x)
#else
#define DMEM_OSALLOCMEM(size,pointer,type) rn_getseg(0,size,RN_NOWAIT,0,(void **)&pointer)
/* Similar, but this returns the memory */
#define DMEM_OSFREEMEM(x) rn_retseg(0, x)
#endif

/* Argument: <id> */
#define SEMAPHOREOBTAIN(x) sm_p(x, SM_WAIT, 0)
/* Argument: <id> */
#define SEMAPHORERETURN(x) sm_v(x)
/* Argument: <name> <id-variable name> */
#define SEMAPHORECREATE(x,y) sm_create(x, 1, SM_FIFO, (ULONG *)&(y))

#else
#ifdef BMALLOC /* use our own big-memory-allocation system */
#define DMEM_OSALLOCMEM(size,pointer,type) pointer=(type)bmalloc(size)
#define DMEM_OSFREEMEM(x) bfree(x)
#elif DEBUG_MALLOC
#define DMEM_OSALLOCMEM(size,pointer,type) pointer=(type)dbgmalloc(size)
#define DMEM_OSFREEMEM(x) dbgfree(x)
#else
#define DMEM_OSALLOCMEM(size,pointer,type) pointer=(type)malloc(size)
#define DMEM_OSFREEMEM(x) free(x)
#endif
#endif


/* the largest memory allocation that is made a FRAGMENT: (grab the highest
   number from the list below) */
#define DMEM_LARGESTSIZE 2032

/* The total size of a BLOCK used for FRAGMENTS
   In order to make this use only *1* even alignment from the big-block-
   allocation-system (possible the bmalloc() system also written by me)
   we need to subtract the [maximum] struct sizes that could get added all
   the way through to the grab from the memory. */
#define DMEM_BLOCKSIZE 4064 /* (4096 - sizeof(struct MemBlock) - 12) */

/* Since the blocksize isn't an even 2^X story anymore, we make a table with
   the FRAGMENT sizes and amounts that fills up a BLOCK nicely */

/* a little 'bc' script that helps us maximize the usage:
   - for 32-bit aligned addresses (SPARC crashes otherwise):
   for(i=20; i<2040; i+=4) { a=4064/i; if(a*i >= 4060) { {i;} } }

   I try to approximate a double of each size, starting with ~20. We don't do
   ODD sizes since several CPU flavours dump core when accessing such
   addresses. We try to do 32-bit aligned to make ALL kinds of CPUs to remain
   happy with us.
   */

static const unsigned short qinfo[]= {
  20, 28, 52, 116, 312, 580, 1016, 2032
};

#define MIN(x,y) ((x)<(y)?(x):(y))

/* ---------------------------------------------------------------------- */
/*                                 Globals                                */
/* ---------------------------------------------------------------------- */

/* keeper of the chain of BLOCKS */
static struct MemTop top[ sizeof(qinfo)/sizeof(qinfo[0]) ];

/* ---------------------------------------------------------------------- */
/*                          Start of the real code                        */
/* ---------------------------------------------------------------------- */

#ifdef DEBUG_MALLOC
/************
 * A few functions that are verbose and tells us about the current status
 * of the dmalloc system
 ***********/

void dmalloc_status(void)
{
    unsigned int i;
    int used;
    int num;
    int totalfree=0;
    struct MemBlock *block;
    for(i=0; i<sizeof(qinfo)/sizeof(qinfo[0]);i++) {
        block = top[i].chain;
        used = 0;
        num = 0;
        while(block) {
            used += top[i].nmax-block->nfree;
            num++;
            block = block->next;
        }
        printf("Q %d (FRAG %4d), USED %4d FREE %4ld (SIZE %4ld) BLOCKS %d\n",
               i, top[i].fragsize, used, top[i].nfree,
               top[i].nfree*top[i].fragsize, num);
        totalfree += top[i].nfree*top[i].fragsize;
    }
    printf("Total unused memory stolen by dmalloc: %d\n", totalfree);
}
#endif

#ifdef DEBUG_VERBOSE
static void dmalloc_failed(size_t size)
{
  printf("*** " __FILE__ " Couldn't allocate %d bytes\n", size);
  dmalloc_status();
}
#else
#define dmalloc_failed(x)
#endif

#ifdef DEBUG_VERBOSE

#define DBG(x) syslog x

void syslog(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

void memchange(void *a, int x)
{
    static int memory=0;
    static int count=0;
    static int max=0;
    if(memory > max)
        max = memory;
    memory += x;
    DBG(("%d. PTR %p / %d TOTAL %d MAX %d\n", ++count, a, x, memory, max));
}
#else
#define DBG(x)
#endif

/****************************************************************************
 *
 * FragBlock()
 *
 * This function makes FRAGMENTS of the BLOCK sent as argument.
 *
 ***************************************************************************/

static void FragBlock(char *memp, int size)
{
    struct MemFrag *frag=(struct MemFrag *)memp;
    struct MemFrag *prev=NULL; /* no previous in the first round */
    int count=0;
    while((count+size) <= DMEM_BLOCKSIZE) {
        frag->next = (struct MemFrag *)((char *)frag + size);
        frag->prev = prev;
        prev = frag;
        (char *)frag += size;
        count += size;
    }
    prev->next = NULL; /* the last one has no next struct */
}

/***************************************************************************
 *
 * dmalloc_initialize();
 *
 * Call before the first dmalloc(). Inits a few memory things.
 *
 **************************************************************************/
void dmalloc_initialize(void)
{
    unsigned int i;
    /* Setup the nmax and fragsize fields of the top structs */
    for(i=0; i< sizeof(qinfo)/sizeof(qinfo[0]); i++) {
        top[i].fragsize = qinfo[i];
        top[i].nmax = DMEM_BLOCKSIZE/qinfo[i];

#ifdef PSOS
        /* for some reason, these aren't nulled from start: */
        top[i].chain = NULL; /* no BLOCKS */
        top[i].nfree = 0;    /* no FRAGMENTS */
#endif
#ifdef SEMAPHORE
        {
            char name[7];
            snprintf(name, 7, "MEM%d", i);
            SEMAPHORECREATE(name, top[i].semaphore_id);
            /* doesn't matter if it failed, we continue anyway ;-( */
        }
#endif
    }
}

/****************************************************************************
 *
 * fragfromblock()
 *
 * This should return a fragment from the block and mark it as used
 * accordingly.
 *
 ***************************************************************************/

static void *fragfromblock(struct MemBlock *block)
{
    /* make frag point to the first free FRAGMENT */
    struct MemFrag *frag = block->first;
    struct MemInfo *mem = (struct MemInfo *)frag;

    /*
     * Remove the FRAGMENT from the list and decrease the free counters.
     */
    block->first = frag->next; /* new first free FRAGMENT */
    
    block->nfree--; /* BLOCK counter */
    block->top->nfree--; /* TOP counter */
      
    /* heal the FRAGMENT list */
    if(frag->prev) {
        frag->prev->next = frag->next;
    }
    if(frag->next) {
        frag->next->prev = frag->prev;
    }
    mem->block = block; /* no block bit set here */
    
    return ((char *)mem)+sizeof(struct MemInfo);
}

/***************************************************************************
 *
 * dmalloc()
 *
 * This needs no explanation. A malloc() look-alike.
 *
 **************************************************************************/

void *malloc(size_t size)
{
    void *mem;

    DBG(("malloc(%d)\n", size));

    /* First, we make room for the space needed in every allocation */
    size += sizeof(struct MemInfo);

    if(size < DMEM_LARGESTSIZE) {
        /* get a FRAGMENT */

        struct MemBlock *block=NULL; /* SAFE */
        struct MemBlock *newblock=NULL; /* SAFE */
        struct MemTop *memtop=NULL; /* SAFE */

        /* Determine which queue to use */
        unsigned int queue;
        for(queue=0; size > qinfo[queue]; queue++)
            ;
        do {
            /* This is the head master of our chain: */
            memtop = &top[queue];

            DBG(("Top info: CHAIN %p FREE %d MAX %d FRAGSIZE %d\n",
                 memtop->chain,
                 memtop->nfree,
                 memtop->nmax,
                 memtop->fragsize));

#ifdef SEMAPHORE
            if(SEMAPHOREOBTAIN(memtop->semaphore_id))
                return NULL; /* failed somehow */
#endif

            /* get the first BLOCK in the chain */
            block = memtop->chain;

            /* check if we have a free FRAGMENT */
            if(memtop->nfree) {
                /* there exists a free FRAGMENT in this chain */

                /**** We'd prefer to not have this loop here! ****/

                /* search for the free FRAGMENT */
                while(!block->nfree)
                    block = block->next; /* check next BLOCK */

                /*
                 * Now 'block' is the first BLOCK with a free FRAGMENT
                 */

                mem = fragfromblock(block);
      
            }
            else {
                /* we do *not* have a free FRAGMENT but need to get us a new
                 * BLOCK */

                DMEM_OSALLOCMEM(DMEM_BLOCKSIZE + sizeof(struct MemBlock),
                                newblock,
                                struct MemBlock *);
                if(!newblock) {
                    if(++queue < sizeof(qinfo)/sizeof(qinfo[0])) {
                        /* There are queues for bigger FRAGMENTS that we
                         * should check before we fail this for real */
#ifdef DEBUG_VERBOSE
                        printf("*** " __FILE__ " Trying a bigger Q: %d\n",
                               queue);
#endif
                        mem = NULL;
                    }
                    else {
                        dmalloc_failed(size- sizeof(struct MemInfo));
                        return NULL; /* not enough memory */
                    }
                }
                else {
                    /* allocation of big BLOCK was successful */
                    MEMINCR(newblock, DMEM_BLOCKSIZE +
                            sizeof(struct MemBlock));

                    memtop->chain = newblock; /* attach this BLOCK to the
                                                 chain */
                    newblock->next = block;   /* point to the previous first
                                                 BLOCK */
                    if(block)
                        block->prev = newblock; /* point back on this new 
                                                   BLOCK */
                    newblock->prev = NULL;    /* no previous */
                    newblock->top  = memtop;  /* our head master */
	
                    /* point to the new first FRAGMENT */
                    newblock->first = (struct MemFrag *)
                        ((char *)newblock+sizeof(struct MemBlock));

                    /* create FRAGMENTS of the BLOCK: */
                    FragBlock((char *)newblock->first, memtop->fragsize);

                    /* fix the nfree counters */
                    newblock->nfree = memtop->nmax;
                    memtop->nfree += memtop->nmax;

                    /* get a FRAGMENT from the BLOCK */
                    mem = fragfromblock(newblock);
                }
            }
#ifdef SEMAPHORE
            SEMAPHORERETURN(memtop->semaphore_id); /* let it go */
#endif
        } while(NULL == mem); /* if we should retry a larger FRAGMENT */
    }
    else {
        /* get a stand-alone BLOCK */
        struct MemInfo *meminfo;

        if(size&1)
            /* don't leave this with an odd size since we'll use that bit for
               information */
            size++;

        DMEM_OSALLOCMEM(size, meminfo, struct MemInfo *);

        if(meminfo) {
            MEMINCR(meminfo, size);
            meminfo->block = (void *)(size|BLOCK_BIT);
            mem = (char *)meminfo + sizeof(struct MemInfo);
        }
        else {
            dmalloc_failed(size);
            mem = NULL;
        }
    }
    return (void *)mem;
}

/***************************************************************************
 *
 * dfree()
 *
 * This needs no explanation. A free() look-alike.
 *
 **************************************************************************/

void free(void *memp)
{
    struct MemInfo *meminfo = (struct MemInfo *)
        ((char *)memp- sizeof(struct MemInfo));

    DBG(("free(%p)\n", memp));

    if(!((size_t)meminfo->block&BLOCK_BIT)) {
        /* this is a FRAGMENT we have to deal with */

        struct MemBlock *block=meminfo->block;
        struct MemTop *memtop = block->top;

#ifdef SEMAPHORE
        SEMAPHOREOBTAIN(memtop->semaphore_id);
#endif

        /* increase counters */
        block->nfree++;
        memtop->nfree++;

        /* is this BLOCK completely empty now? */
        if(block->nfree == memtop->nmax) {
            /* yes, return the BLOCK to the system */
            if(block->prev)
                block->prev->next = block->next;
            else
                memtop->chain = block->next;
            if(block->next)
                block->next->prev = block->prev;
            
            memtop->nfree -= memtop->nmax; /* total counter subtraction */
            MEMDECR(block, DMEM_BLOCKSIZE + sizeof(struct MemBlock));
            DMEM_OSFREEMEM((void *)block); /* return the whole block */
        }
        else {
            /* there are still used FRAGMENTS in the BLOCK, link this one
               into the chain of free ones */
            struct MemFrag *frag = (struct MemFrag *)meminfo;
            frag->prev = NULL;
            frag->next = block->first;
            if(block->first)
                block->first->prev = frag;
            block->first = frag;
        }
#ifdef SEMAPHORE
        SEMAPHORERETURN(memtop->semaphore_id);
#endif
    }
    else {
        /* big stand-alone block, just give it back to the OS: */

        /* clean BLOCK_BIT */
        MEMDECR(meminfo->block, (size_t)meminfo->block&~BLOCK_BIT);
        DMEM_OSFREEMEM((void *)meminfo);
    }
}

/***************************************************************************
 *
 * drealloc()
 *
 * This needs no explanation. A realloc() look-alike.
 *
 **************************************************************************/

void *realloc(void *ptr, size_t size)
{
    struct MemInfo *meminfo = (struct MemInfo *)
        ((char *)ptr- sizeof(struct MemInfo));
    /*
     * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     * NOTE: the ->size field of the meminfo will now contain the MemInfo
     * struct size too!
     * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     */
    void *mem=NULL; /* SAFE */
    size_t prevsize;

    /* NOTE that this is only valid if BLOCK_BIT isn't set: */
    struct MemBlock *block;

    DBG(("realloc(%p, %d)\n", ptr, size));

    if(NULL == ptr)
        return malloc( size );

    block = meminfo->block;

    /* Here we check if this is a FRAGMENT and if the new size is
       still smaller than the fragsize for this block. */
    if(!((size_t)block&BLOCK_BIT) &&
       (size + sizeof(struct MemInfo) < block->top->fragsize )) {

        prevsize = block->top->fragsize;
        /* This is a FRAGMENT and new size is possible to retain within the
           same FRAGMENT */
        if((prevsize > qinfo[0]) &&
           /* this is not the smallest memory Q */
           (size < (block->top-1)->fragsize))
            /* This fits in a smaller fragment, so we will make a realloc
               here */
            ;
        else
            mem = ptr; /* Just return the same pointer as we got in. */
    }
    if(!mem) {
        if((size_t)meminfo->block&BLOCK_BIT) {
            /* This is a stand-alone BLOCK */
            prevsize = ((size_t)meminfo->block&~BLOCK_BIT) -
                sizeof(struct MemInfo);
        }
        else
            /* a FRAGMENT realloc that no longer fits within the same FRAGMENT
             * or one that fits in a smaller */
            prevsize = block->top->fragsize;

        /* No tricks involved here, just grab a new bite of memory, copy the
         * data from the old place and free the old memory again. */
        mem = malloc(size);
        if(mem) {
            memcpy(mem, ptr, MIN(size, prevsize) );
            free(ptr);
        }
    }
    return mem;
}

/***************************************************************************
 *
 * dcalloc()
 *
 * This needs no explanation. A calloc() look-alike.
 *
 **************************************************************************/
/* Allocate an array of NMEMB elements each SIZE bytes long.
   The entire array is initialized to zeros.  */
void *
calloc (size_t nmemb, size_t size)
{
    void *result = malloc (nmemb * size);
    
    if (result != NULL)
        memset (result, 0, nmemb * size);
  
    return result;
}

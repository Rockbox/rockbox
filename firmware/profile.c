/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Profiling routines counts ticks and calls to each profiled function.
 * 
 * Copyright (C) 2005 by Brandon Low
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************
 *
 * profile_func_enter() based on mcount found in gmon.c:
 *
 ***************************************************************************
 * Copyright (c) 1991, 1998 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. [rescinded 22 July 1999]
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * @(#)gmon.c 5.3 (Berkeley) 5/22/91
 */

#include <file.h>
#include <sprintf.h>
#include <system.h>
#include <string.h>
#include <timer.h>
#include "profile.h"

static unsigned short profiling = PROF_OFF;
static size_t recursion_level;
static unsigned short indices[INDEX_SIZE];
static struct pfd_struct pfds[NUMPFDS];
/* This holds a pointer to the last pfd effected for time tracking */
static struct pfd_struct *last_pfd;
/* These are used to track the time when we've lost the CPU so it doesn't count
 * against any of the profiled functions */
static int profiling_thread = -1;

/* internal function prototypes */
static void profile_timer_tick(void);
static void profile_timer_unregister(void);

static void write_function_recursive(int fd, struct pfd_struct *pfd, int depth);

/* Be careful to use the right one for the size of your variable */
#define ADDQI_L(_var,_value) \
    asm ("addq.l %[value],%[var];" \
         : [var] "+g" (_var) \
         : [value] "I" (_value) )

void profile_thread_stopped(int current_thread) {
    if (current_thread == profiling_thread) {
        /* If profiling is busy or idle */
        if (profiling < PROF_ERROR) {
            /* Unregister the timer so that other threads aren't interrupted */
            timer_unregister();
        }
        /* Make sure we don't waste time profiling when we're running the
         * wrong thread */
        profiling |= PROF_OFF_THREAD;
    }
}

void profile_thread_started(int current_thread) {
    if (current_thread == profiling_thread) {
        /* Now we are allowed to profile again */
        profiling &= PROF_ON_THREAD;
        /* if profiling was busy or idle */
        if (profiling < PROF_ERROR) {
            /* After we de-mask, if profiling is active, reactivate the timer */
            timer_register(0, profile_timer_unregister, 
                    CPU_FREQ/10000, 0, profile_timer_tick);
        }
    }
}

static void profile_timer_tick(void) {
    if (!profiling) {
        register struct pfd_struct *my_last_pfd = last_pfd;
        if (my_last_pfd) {
            ADDQI_L(my_last_pfd->time,1);
        }
    }
}

static void profile_timer_unregister(void) {
    profiling = PROF_ERROR;
    profstop();
}

/* This function clears the links on top level linkers, and clears the needed
 * parts of memory in the index array */
void profstart(int current_thread) {
    recursion_level = 0;
    profiling_thread = current_thread;
    last_pfd = (struct pfd_struct*)0;
    pfds[0].link = 0;
    pfds[0].self_pc = 0;
    memset(&indices,0,INDEX_SIZE * sizeof(unsigned short));
    timer_register(
        0, profile_timer_unregister, CPU_FREQ/10000, 0, profile_timer_tick);
    profiling = PROF_ON;
}

static void write_function_recursive(int fd, struct pfd_struct *pfd, int depth){
    unsigned short link = pfd->link;
    fdprintf(fd,"0x%08lX\t%08ld\t%08ld\t%04d\n", (size_t)pfd->self_pc, 
            pfd->count, pfd->time, depth);
    if (link > 0 && link < NUMPFDS) { 
        write_function_recursive(fd, &pfds[link], depth++);
    }
}

void profstop() {
    int profiling_exit = profiling;
    int fd = 0;
    int i;
    unsigned short current_index;
    timer_unregister();
    profiling = PROF_OFF;
    fd = open("/profile.out", O_WRONLY|O_CREAT|O_TRUNC);
    if (profiling_exit == PROF_ERROR) {
        fdprintf(fd,"Profiling exited with an error.\n");
        fdprintf(fd,"Overflow or timer stolen most likely.\n");
    }
    fdprintf(fd,"PROFILE_THREAD\tPFDS_USED\n");
    fdprintf(fd,"%08d\t%08d\n", profiling_thread,
        pfds[0].link);
    fdprintf(fd,"FUNCTION_PC\tCALL_COUNT\tTICKS\t\tDEPTH\n");
    for (i = 0; i < INDEX_SIZE; i++) {
        current_index = indices[i];
        if (current_index != 0) { 
            write_function_recursive(fd, &pfds[current_index], 0);
        }
    }
    fdprintf(fd,"DEBUG PROFILE DATA FOLLOWS\n");
    fdprintf(fd,"INDEX\tLOCATION\tSELF_PC\t\tCOUNT\t\tTIME\t\tLINK\tCALLER\n");
    for (i = 0; i < NUMPFDS; i++) {
        struct pfd_struct *my_last_pfd = &pfds[i];
        if (my_last_pfd->self_pc != 0) {
            fdprintf(fd,
                    "%04d\t0x%08lX\t0x%08lX\t0x%08lX\t0x%08lX\t%04d\t0x%08lX\n",
                    i, (size_t)my_last_pfd, (size_t)my_last_pfd->self_pc,
                    my_last_pfd->count, my_last_pfd->time, my_last_pfd->link,
                    (size_t)my_last_pfd->caller);
        }
    }
    fdprintf(fd,"INDEX_ADDRESS=INDEX\n");
    for (i=0; i < INDEX_SIZE; i++) {
        fdprintf(fd,"%08lX=%04d\n",(size_t)&indices[i],indices[i]);
    }
    close(fd);
}

void profile_func_exit(void *self_pc, void *call_site) {
    (void)call_site;
    (void)self_pc;
    /* When we started timing, we set the time to the tick at that time
     * less the time already used in function */
    if (profiling) {
        return;
    }
    profiling = PROF_BUSY;
    {
        register unsigned short my_recursion_level = recursion_level;
        if (my_recursion_level) {
            my_recursion_level--;
            recursion_level = my_recursion_level;
        } else {
            /* This shouldn't be necessary, maybe exit could be called first */
            register struct pfd_struct *my_last_pfd = last_pfd;
            if (my_last_pfd) {
                last_pfd = my_last_pfd->caller;
            }
        }
    }
    profiling = PROF_ON;
}

#define ALLOCATE_PFD(temp) \
    temp = ++pfds[0].link;\
    if (temp >= NUMPFDS) goto overflow; \
    pfd = &pfds[temp];\
    pfd->self_pc = self_pc; pfd->count = 1; pfd->time = 0

void profile_func_enter(void *self_pc, void *from_pc) {
    struct pfd_struct *pfd;
    struct pfd_struct *prev_pfd;
    unsigned short *pfd_index_pointer;
    unsigned short pfd_index;

    /* check that we are profiling and that we aren't recursively invoked
     * this is equivalent to 'if (profiling != PROF_ON)' but it's faster */
    if (profiling) {
        return;
    }
    /* this is equivalent to 'profiling = PROF_BUSY;' but it's faster */
    profiling = PROF_BUSY;
    /* A check that the PC is in the code range here wouldn't hurt, but this is
     * logically guaranteed to be a valid address unless the constants are
     * breaking the rules.  */
    pfd_index_pointer = &indices[((size_t)from_pc)&INDEX_MASK];
    pfd_index = *pfd_index_pointer;
    if (pfd_index == 0) {
        /* new caller, allocate new storage */
        ALLOCATE_PFD(pfd_index);
        pfd->link = 0;
        *pfd_index_pointer = pfd_index;
        goto done;
    }
    pfd = &pfds[pfd_index];
    if (pfd->self_pc == self_pc) {
        /* only / most recent function called by this caller, usual case */
        /* increment count, start timing and exit */
        goto found;
    }
    /* collision, bad for performance, look down the list of functions called by
     * colliding PCs */
    for (; /* goto done */; ) {
        pfd_index = pfd->link;
        if (pfd_index == 0) {
            /* no more previously called functions, allocate a new one */
            ALLOCATE_PFD(pfd_index);
            /* this function becomes the new head, link to the old head */
            pfd->link = *pfd_index_pointer;
            /* and set the index to point to this function */
            *pfd_index_pointer = pfd_index;
            /* start timing and exit */
            goto done;
        }
        /* move along the chain */
        prev_pfd = pfd;
        pfd = &pfds[pfd_index];
        if (pfd->self_pc == self_pc) {
            /* found ourself */
            /* Remove me from my old spot */
            prev_pfd->link = pfd->link;
            /* Link to the old head */
            pfd->link = *pfd_index_pointer;
            /* Make me head */
            *pfd_index_pointer = pfd_index;
            /* increment count, start timing and exit */
            goto found;
        }

    }
/* We've found a pfd, increment it */
found:
    ADDQI_L(pfd->count,1);
/* We've (found or created) and updated our pfd, save it and start timing */
done:
    {
        register struct pfd_struct *my_last_pfd = last_pfd;
        if (pfd != my_last_pfd) {
            /* If we are not recursing */
            pfd->caller = my_last_pfd;
            last_pfd = pfd;
        } else {
            ADDQI_L(recursion_level,1);
        }
    }
    /* Start timing this function */
    profiling = PROF_ON;
    return;  /* normal return restores saved registers */

overflow:
    /* this is the same as 'profiling = PROF_ERROR' */
    profiling = PROF_ERROR;
    return;
}

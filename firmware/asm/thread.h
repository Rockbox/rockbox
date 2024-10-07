/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Ulf Ralberg
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

#ifndef __ASM_THREAD_H__
#define __ASM_THREAD_H__
#include "config.h"

/* generic thread.h */

#if defined(HAVE_WIN32_FIBER_THREADS) || defined(HAVE_SIGALTSTACK_THREADS)

struct regs
{
    void (*start)(void); /* thread's entry point, or NULL when started */
    void* uc;            /* host thread handle */
    uintptr_t sp;        /* Stack pointer, unused */
    size_t stack_size;   /* stack size, not always used */
    uintptr_t stack;     /* pointer to start of the stack buffer */
};
  #include <errno.h>
  #ifdef HAVE_SIGALTSTACK_THREADS
    #include <signal.h>
    #ifdef _DYNAMIC_STACK_SIZE_SOURCE
    /* glibc 2.34 made MINSIGSTKSZ non-constant. This is a problem for sim
     * builds. Hosted targets are using ancient glibc where MINSIGSTKSZ is
     * still a compile time constant. On platforms where this is a problem
     * (mainly x86-64 and ARM64) the signal stack size can be big, so let's
     * give a decent amount of space and hope for the best...
     * FIXME: this isn't a great solution. */
    #undef MINSIGSTKSZ
    #endif
    #ifndef MINSIGSTKSZ
      #define MINSIGSTKSZ 16384
    #endif
    /* MINSIGSTKSZ for the OS to deliver the signal, plus more for us */
#if defined(SIMULATOR) || defined(__aarch64__)
    #define DEFAULT_STACK_SIZE (MINSIGSTKSZ+0x6000) /* Bytes */
#else
    #define DEFAULT_STACK_SIZE (MINSIGSTKSZ+0x3000) /* Bytes */
#endif
  #elif defined(HAVE_WIN32_FIBER_THREADS)
    #define DEFAULT_STACK_SIZE 0x1000 /* Bytes */
  #endif
#elif defined(CPU_ARM)
  #include "arm/thread.h"
#elif defined(CPU_COLDFIRE)
  #include "m68k/thread.h"
#elif defined(CPU_MIPS)
  #include "mips/thread.h"
#endif

#endif

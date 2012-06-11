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

  #ifdef HAVE_SIGALTSTACK_THREADS
    #include <signal.h>
    /* MINSIGSTKSZ for the OS to deliver the signal + 0x3000 for us */
    #define DEFAULT_STACK_SIZE (MINSIGSTKSZ+0x3000) /* Bytes */
  #elif defined(HAVE_WIN32_FIBER_THREADS)
    #define DEFAULT_STACK_SIZE 0x1000 /* Bytes */
  #endif
#elif defined(CPU_ARM)
  #include "arm/thread.h"
#elif defined(CPU_COLDFIRE)
  #include "m68k/thread.h"
#elif CONFIG_CPU == SH7034
  #include "sh/thread.h"
#elif defined(CPU_MIPS)
  #include "mips/thread.h"
#endif

#endif

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

struct regs
{
    uint32_t r[9];  /* 0-32 - Registers s0-s7, fp */
    uint32_t sp;    /*   36 - Stack pointer */
    uint32_t ra;    /*   40 - Return address */
    uint32_t start; /*   44 - Thread start address, or NULL when started */
};

#ifdef HAVE_FPU
struct fpregs
{
    uint32_t fp[32]; /* 0-124 - Floating point registers f0-f31 */
    uint32_t fcr31;  // 128
    uint32_t msacsr; // 132
};
#define __FPU_HAZARD "ssnop; ssnop; ssnop; ehb;"
#define __SYSTEM_FPU_ENABLE()  set_c0_status(ST0_CU1);
#define __SYSTEM_FPU_DISABLE() clear_c0_status(ST0_CU1); // __FPU_HAZARD();
#endif

#define DEFAULT_STACK_SIZE 0x400 /* Bytes */

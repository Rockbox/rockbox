/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#ifndef __HWSTUB_CONFIG__
#define __HWSTUB_CONFIG__

#include "target-config.h"

#ifndef STACK_SIZE
#define STACK_SIZE      0x1000
#endif
#define MAX_LOGF_SIZE   128

#if defined(CPU_ARM) && defined(__ASSEMBLER__)
/* ARMv4T doesn't switch the T bit when popping pc directly, we must use BX */
.macro ldmpc cond="", order="ia", regs
#if ARM_ARCH == 4 && defined(USE_THUMB)
    ldm\cond\order sp!, { \regs, lr }
    bx\cond lr
#else
    ldm\cond\order sp!, { \regs, pc }
#endif
.endm
.macro ldrpc cond=""
#if ARM_ARCH == 4 && defined(USE_THUMB)
    ldr\cond lr, [sp], #4
    bx\cond  lr
#else
    ldr\cond pc, [sp], #4
#endif
.endm
#endif

#endif /* __HWSTUB_CONFIG__ */

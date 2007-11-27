/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Greg White
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#include "mmu-imx31.h"
#include "system-arm.h"

#define CPUFREQ_NORMAL 532000000

static inline void udelay(unsigned int usecs)
{
    volatile signed int stop = EPITCNT1 - usecs;
    while (EPITCNT1 > stop);
}

#define __dbg_hw_info(...) 0
#define __dbg_ports(...) 0

#define HAVE_INVALIDATE_ICACHE
static inline void invalidate_icache(void)
{
}

struct ARM_REGS {
	int r0;
	int r1;
	int r2;
	int r3;
	int r4;
	int r5;
	int r6;
	int r7;
	int r8;
	int r9;
	int r10;
	int r11;
	int r12;
	int sp;
	int lr;
	int pc;
	int cpsr;
} regs;

inline void dumpregs(void);

#endif /* SYSTEM_TARGET_H */

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2008 Michael Sevakis
 *
 * Clock control functions for IMX31 processor
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "system.h"
#include "cpu.h"
#include "clkctl-imx31.h"

void imx31_clkctl_module_clock_gating(enum IMX31_CG_LIST cg,
                                      enum IMX31_CG_MODES mode)
{
    volatile unsigned long *reg;
    unsigned long mask;
    int shift;
    int oldlevel;

    if (cg >= CG_NUM_CLOCKS)
        return;

    reg = &CLKCTL_CGR0 + cg / 16;   /* Select CGR0, CGR1, CGR2 */
    shift = 2*(cg % 16);            /* Get field shift */
    mask = CG_MASK << shift;        /* Select field */

    oldlevel = disable_interrupt_save(IRQ_FIQ_STATUS);

    *reg = (*reg & ~mask) | ((mode << shift) & mask);

    restore_interrupt(oldlevel);
}

/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2005 Jens Arnold
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdbool.h>
#include "config.h"

#if defined(CPU_PP)
 /* Portalplayer chips use a microsecond timer. */
 #define TIMER_FREQ 1000000
#elif defined(CPU_COLDFIRE)
 /* timer is based on busclk == cpuclk/2 */
 #define TIMER_FREQ (CPU_FREQ/2)
#elif CONFIG_CPU == PNX0101
 #define TIMER_FREQ 3000000
#elif CONFIG_CPU == S3C2440 || CONFIG_CPU == DM320 || CONFIG_CPU == TCC7801 \
      || defined(CPU_TCC77X)
 #include "timer-target.h"
#elif defined(SIMULATOR)
 #define TIMER_FREQ 1000000
#else
 #define TIMER_FREQ CPU_FREQ
#endif
bool timer_register(int reg_prio, void (*unregister_callback)(void),
                    long cycles, int int_prio, void (*timer_callback)(void)
                    IF_COP(,int core));
bool timer_set_period(long cycles);
#ifdef CPU_COLDFIRE
void timers_adjust_prescale(int multiplier, bool enable_irq);
#endif
void timer_unregister(void);

/* For target-specific interface use */
extern void (*pfn_timer)(void);
extern void (*pfn_unregister)(void);

#endif /* __TIMER_H__ */

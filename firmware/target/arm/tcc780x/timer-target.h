/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2007 by Karl Kurbjun
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/
#ifndef TIMER_TARGET_H
#define TIMER_TARGET_H

/* timers are based on XIN (12Mhz) */
#define TIMER_FREQ (12000000)

bool __timer_set(long cycles, bool set);
bool __timer_register(void);
void __timer_unregister(void);

#define __TIMER_SET(cycles, set) \
    __timer_set(cycles, set)

#define __TIMER_REGISTER(reg_prio, unregister_callback, cycles, \
                              int_prio, timer_callback) \
    __timer_register()

#define __TIMER_UNREGISTER(...) \
    __timer_unregister()

#endif /* TIMER_TARGET_H */

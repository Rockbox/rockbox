/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Mauricio G.
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
#ifndef __SYSIRQ_PSP_H__
#define __SYSIRQ_PSP_H__

/* PSP only: interrupt emulation for the native scheduler (sys_irq_psp.c).
 * It also provides set_irq_level(), sim_enter_irq_handler(),
 * sim_exit_irq_handler() and wait_for_interrupt() (system-console.h). */
void sys_irq_init(void);
void sys_irq_quit(void);

#endif /* __SYSIRQ_PSP_H__ */

/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 *
 * Copyright (C) 2025 by Mauricio G.
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
#ifndef _SYSTEM_CONSOLE_H_
#define _SYSTEM_CONSOLE_H_

#include <stdbool.h>

#include "config.h"
#include "gcc_extensions.h"

#ifdef SONY_PSP
/* Use a big enough stack for threads to run but
   we must keep stack size under 256 kB in total.
   4096 bytes seems like a good default value */
#define DEFAULT_STACK_SIZE 0x1000

/**
 * Absolute maximum CPU speed: 333 MHz
 * Intermediate CPU speeds: 222 Mhz, 111 MHz
 * Absolute minimum CPU speed: 19 MHz */
#define PSP_CPUFREQ_333_MHz     333
#define PSP_CPUFREQ_222_MHz     222
#define PSP_CPUFREQ_111_MHz     111
#define PSP_CPUFREQ_33_MHz      33

#define CPUFREQ_DEFAULT     PSP_CPUFREQ_222_MHz
#define CPUFREQ_NORMAL      PSP_CPUFREQ_222_MHz
#define CPUFREQ_MAX         PSP_CPUFREQ_333_MHz
#define CPUFREQ_SLEEP       PSP_CPUFREQ_33_MHz
#endif

#define HIGHEST_IRQ_LEVEL 1

int set_irq_level(int level);

#define disable_irq() \
    ((void)set_irq_level(HIGHEST_IRQ_LEVEL))

#define enable_irq()  \
    ((void)set_irq_level(0))

#define disable_irq_save() \
    set_irq_level(HIGHEST_IRQ_LEVEL)

#define restore_irq(level) \
    ((void)set_irq_level(level))

#define wait_for_interrupt()

#include "system-hosted.h"

void sim_enter_irq_handler(void);
void sim_exit_irq_handler(void);
void sim_kernel_shutdown(void);
void sys_poweroff(void);
void sys_handle_argv(int argc, char *argv[]);
void gui_message_loop(void);
void sim_do_exit(void) NORETURN_ATTR;
void console_sys_quit(void);

void mcuhwc_init(void);
void mcuhwc_close(void);

extern bool background;  /* True if the background image is enabled */
extern bool showremote;
extern double display_zoom;
extern long start_tick;

#endif /* _SYSTEM_CONSOLE_H_ */

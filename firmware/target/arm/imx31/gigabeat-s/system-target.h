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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef SYSTEM_TARGET_H
#define SYSTEM_TARGET_H

#include "system-arm.h"
#include "mmu-arm.h"

/* High enough for most tasks but low enough for reduced voltage */
#define CPUFREQ_DEFAULT     264000000
/* Still quite powerful, minimum possible frequency */
#define CPUFREQ_NORMAL      132000000
/* Overdrive mode */
#define CPUFREQ_MAX         528000000

#define CPU_MULTI_FREQUENCY 3

static inline void udelay(unsigned long usecs)
{
    unsigned long stop = GPTCNT + usecs;
    while (TIME_BEFORE(GPTCNT, stop));
}

static inline unsigned long usec_timer(void)
{
    return GPTCNT;
}

/* Fire an event usecs microseconds from now */
typedef void (* uevent_cb_type)(void);
void uevent(unsigned long usecs, uevent_cb_type callback);
void uevent_cancel(void);

void watchdog_init(unsigned int half_seconds);
void watchdog_service(void);

void gpt_start(void);
void gpt_stop(void);

unsigned int iim_system_rev(void);

/* Prepare for transition to (new) firmware */
void system_prepare_fw_start(void);
void tick_stop(void);
void kernel_device_init(void);
void system_halt(void);

#define KDEV_INIT

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

void dumpregs(void);

bool usb_plugged(void);
void usb_connect_event(void);

/** Sector read/write filters **/

/* Filter some things in the MBR - see usb-gigabeat-s.c */
void usb_fix_mbr(unsigned char *mbr);
#define USBSTOR_READ_SECTORS_FILTER() \
    ({ if (cur_cmd.sector == 0) \
            usb_fix_mbr(cur_cmd.data[cur_cmd.data_select]); \
    0; })

/* Disallow MBR writes entirely since it was "fixed" in usb_fix_mbr */
#define USBSTOR_WRITE_SECTORS_FILTER() \
    ({ cur_cmd.sector != 0 ? 0 : -1; })

#endif /* SYSTEM_TARGET_H */

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Gigabeat specific code for the Wolfson codec
 *
 * Based on code from the ipodlinux project - http://ipodlinux.org/
 * Adapted for Rockbox in December 2005
 *
 * Original file: linux/arch/armnommu/mach-ipod/audio.c
 *
 * Copyright (c) 2003-2005 Bernard Leach (leachbj@bouncycastle.org)
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
#include "cpu.h"
#include "kernel.h"
#include "sound.h"
#include "i2c-meg-fx.h"
#include "system-target.h"
#include "timer-target.h"
#include "wmcodec.h"

#ifdef HAVE_HARDWARE_BEEP
static void beep_stop(void);
#endif

void audiohw_init(void)
{
    /* GPC5 controls headphone output */
    GPCCON &= ~(0x3 << 10);
    GPCCON |= (1 << 10);
    GPCDAT |= (1 << 5);

    audiohw_preinit();

#ifdef HAVE_HARDWARE_BEEP
    /* pin pullup ON */
    GPBUP &= ~(1 << 3);
    beep_stop();
    /* set pin to TIMER3 output (functional TOUT3) */
    GPBCON = (GPBCON & ~(0x3 << 6)) | (2 << 6);
#endif
}

void wmcodec_write(int reg, int data)
{
    unsigned char d[2];
    d[0] = (reg << 1) | ((data & 0x100) >> 8);
    d[1] = data;
    i2c_write(0x34, d, 2);
}

#ifdef HAVE_HARDWARE_BEEP
/** Beeping via TIMER3 output to codec MONO input **/
static int beep_cycles = 0;
static int beep_cycle_count = 0;

static void beep_stop(void)
{
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);

    /* stop interrupt */
    INTMSK |= TIMER3_MASK;
    /* stop timer */
    TCON &= ~(1 << 16);
    /* be sure timer PWM pin is LOW to avoid noise */
    TCON ^= (GPBDAT & (1 << 3)) << 15;
    /* clear pending */
    SRCPND = TIMER3_MASK;
    INTPND = TIMER3_MASK;

    restore_interrupt(oldstatus);
}

/* Timer interrupt called on every cycle */
void TIMER3(void)
{
    if (++beep_cycles >= beep_cycle_count)
    {
        /* beep is complete */
        beep_stop();
    }

    /* clear pending */
    SRCPND = TIMER3_MASK;
    INTPND = TIMER3_MASK;
}

void pcmbuf_beep(unsigned int frequency, size_t duration, int amplitude)
{
    #define TIMER3_TICK_SEC (TIMER_FREQ / TIMER234_PRESCALE)

    unsigned long tcnt, tcmp;
    int oldstatus;

    if (amplitude <= 0)
    {
        beep_stop(); /* won't hear it anyway */
        return;
    }

    /* pretend this is pcm */
    if (amplitude > 32767)
        amplitude = 32767;

    /* limit frequency range to keep math in range */
    if (frequency > 19506)
        frequency = 19506;
    else if (frequency < 18)
        frequency = 18;

    /* set timer */
    tcnt = TIMER3_TICK_SEC / frequency;

    oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);

    beep_cycles = 0;
    beep_cycle_count = TIMER3_TICK_SEC * duration / (tcnt*1000);

    /* divider = 1/2 */
    TCFG1 = (TCFG1 & ~(0xf << 12)) | (0 << 12);
    /* stop TIMER3, inverter OFF */
    TCON &= ~((1 << 18) | (1 << 16));
    /* set countdown */
    TCNTB3 = tcnt;
    /* set PWM counter - control volume with duty cycle. */
    tcmp = tcnt*amplitude / (65536*2 - 2*amplitude);
    TCMPB3 = tcmp < 1 ? 1 : tcmp;
    /* manual update: on (to reset count), interval mode (auto reload) */
    TCON |= (1 << 19) | (1 << 17);
    /* clear manual bit */
    TCON &= ~(1 << 17);
    /* start timer */
    TCON |= (1 << 16);
    /* enable timer interrupt */
    INTMSK &= ~TIMER3_MASK;

    restore_interrupt(oldstatus);
}
#endif /* HAVE_HARDWARE_BEEP */

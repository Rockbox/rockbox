/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
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
#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "pcf50606.h"
#include "backlight.h"
#include "backlight-target.h"

#ifndef SIMULATOR

void power_init(void)
{
    /* Initialize IDE power pin */
    GPGCON=( GPGCON&~(1<<23) ) | (1<<22); /* Make the pin an output */
    GPGUP |= 1<<11;  /* Disable pullup in SOC as we are now driving */
    ide_power_enable(true);
    /* Charger detect */
}

bool charger_inserted(void)
{
    return (GPFDAT & (1 << 4)) ? false : true;
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void) {
    return (GPGDAT & (1 << 8)) ? false : true;
}

void ide_power_enable(bool on)
{
    if (on)
        GPGDAT |= (1 << 11);
    else
        GPGDAT &= ~(1 << 11);
}

bool ide_powered(void)
{
    return (GPGDAT & (1 << 11)) != 0;
}

void power_off(void)
{
    int(*reboot_point)(void);
    reboot_point=(void*)(unsigned char*) 0x00000000;
    /* turn off backlight and wait for 1 second */
    _backlight_off();
    _buttonlight_off();
    sleep(HZ);
    
    /* Do this to allow the drive to properly reset when player restarts 
     * immediately without running OF shutdown.
     */
    GPGCON&=~0x00300000; 

    /* Rockbox never properly shutdown the player.  When the sleep bit is set
     * the player actually wakes up in some type of "zombie" state 
     * because the shutdown routine is not set up properly.  So far the
     * shutdown routines tried leave the player consuming excess power
     * so we rely on the OF to shut everything down instead. (mmu apears to be
     * reset when the sleep bit is set)
     */  
    CLKCON |=(1<<3);

    reboot_point();

#if 0
    
    GPBCON=0x00015450;
    GPBDAT=0x403;
    GPBUP=0x3FD;

    GPCCON  =0xAAA054A8;
    GPCDAT  =0x0000038C;
    GPCUP   =0xFFFF;
    
    
    GPDCON  =0xAAA0AAA5;
    GPDDAT  =0x00000300;
    GPDUP   =0xFCFF;

    
    GPECON  =0xAA8002AA;
    GPEDAT  =0x0000FFED;
    GPEUP   =0x3817;

    GPFCON  =0x00000a00; 
    GPFDAT  =0x000000F1;
    GPFUP   =0x000000FF;

    GPGCON  =0x01401002;
    GPGDAT  =0x00000180;
    GPGUP   =0x0000FF7F;

    GPHCON  =0x001540A5;
    GPHDAT  =0x000006FD;
    GPHUP   =0x00000187;

// mine
    INTMSK  =0xFFFFFFFF;
    EINTMASK=0x0FFFFEF0;
    EXTINT0 =0xFFFFFECF;
    EXTINT1 =0x07;
//

//    INTMSK=0xFFFFFFFF;
//    EINTMASK=0x00200000;

//    GPHDAT=0x00000004;

//    EXTINT0=~0x00000130;
//    INTMSK=(~0x00000130)+0x00000100;
//    GPGUP=0xFFFFFFFF;

//mine
    INTMSK  =0xFFFFFFDE;
//

    SRCPND=0xFFFFFFFF;
    INTPND=0xFFFFFFFF;
    GSTATUS1=0x00000600;

    ADCCON=0x00000004;

//    MISCCR=MISCCR&(~0x703000)|0x603000;
    LCDCON1=0x00000000;
    LOCKTIME=0xFFFFFFFF;
//    REFRESH=REFRESH|0x00400000;

//    MISCCR=MISCCR|0x000E0000;

//    CLKCON=CLKCON|0x00004018;

    /* 
     * This next piece of code was taken from the linux 2.6.17 sources:
     * linux/arch/arm/mach-s3c2410/sleep.S
     *
     * Copyright (c) 2004 Simtec Electronics
     *      Ben Dooks <ben@simtec.co.uk>
     * 
     * Based on PXA/SA1100 sleep code by:
     *      Nicolas Pitre, (c) 2002 Monta Vista Software Inc
     *      Cliff Brake, (c) 2001
     */

    asm volatile 
    (
        /* get REFRESH, MISCCR, and CLKCON (and ensure in TLB) */
        "ldr   r4, =0x48000024      \n"
        "ldr   r5, =0x56000080      \n"
        "ldr   r6, =0x4C00000C      \n"
        "ldr   r7, [ r4 ]           \n"
        "ldr   r8, [ r5 ]           \n" 
        "ldr   r9, [ r6 ]           \n" 

        /* Setup register writes */
        "ldr   r2, =0x006E3000      \n"
        "ldr   r3, =0x00004018      \n"
        "orr   r7, r7, #0x00400000  \n" /* SDRAM sleep command */
        "orr   r8, r8, r2           \n" /* SDRAM power-down signals */
        "orr   r9, r9, r3           \n" /* power down command */

        /* first as a trial-run to load cache */
        "teq   pc, #0               \n"         
        "bl    s3c2410_do_sleep     \n"

        /* now do it for real */
        "teq   r0, r0               \n" 
        "b     s3c2410_do_sleep     \n"

        /* align next bit of code to cache line */
        ".align  8                  \n"
        "s3c2410_do_sleep:          \n"
        "streq r7, [ r4 ]           \n" /* SDRAM sleep command */
        "streq r8, [ r5 ]           \n" /* SDRAM power-down config */
        "streq r3, [ r6 ]           \n" /* CPU sleep */
        "1:                         \n"
        "beq   1b                   \n"
        "bx lr                      \n"
    );
#endif
}

#else /* SIMULATOR */

bool charger_inserted(void)
{
    return false;
}

void charger_enable(bool on)
{
    (void)on;
}

void power_off(void)
{
}

void ide_power_enable(bool on)
{
   (void)on;
}

#endif /* SIMULATOR */


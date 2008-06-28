/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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
#include "kernel.h"
#include "thread.h"
#include "system.h"
#include "power.h"
#include "panic.h"
#include "ata-target.h"
#include "dm320.h"

void sleep_ms(int ms)
{
    sleep(ms*HZ/1000);
}

void ide_power_enable(bool on)
{
/* Disabled until figured out what's wrong */
#if 0
    int old_level = disable_irq_save();
    if(on)
    {
        IO_GIO_BITSET0 = (1 << 14);
        ata_reset();
    }
    else
        IO_GIO_BITCLR0 = (1 << 14);
    restore_irq(old_level);
#else
    (void)on;
#endif
}

inline bool ide_powered()
{
#if 0
    return (IO_GIO_BITSET0 & (1 << 14));
#else
    return true;
#endif
}

void ata_reset(void)
{
    int old_level = disable_irq_save();
    if(!ide_powered())
    {
        ide_power_enable(true);
        sleep_ms(150);
    }
    else
    {
        IO_GIO_BITSET0 = (1 << 5);
        IO_GIO_BITCLR0 = (1 << 3);
        sleep_ms(1);
    }
    IO_GIO_BITCLR0 = (1 << 5);
    sleep_ms(10);
    IO_GIO_BITSET0 = (1 << 3);
    while(!(ATA_COMMAND & STATUS_RDY))
        sleep_ms(10);
    restore_irq(old_level);
}

void ata_enable(bool on)
{
    (void)on;
    return;
}

bool ata_is_coldstart(void)
{
    return true;
}

void ata_device_init(void)
{
    IO_INTC_EINT1 |= INTR_EINT1_EXT2;   /* enable GIO2 interrupt */
    //TODO: mimic OF inits...
    return;
}

void GIO2(void)
{
#ifdef DEBUG
    logf("GIO2 interrupt...");
#endif
    IO_INTC_IRQ1 = INTR_IRQ1_EXT2; /* Mask GIO2 interrupt */
    return;
}

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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

void ide_power_enable(bool on){
#if 0
    IO_INTC_EINT1 &= ~INTR_EINT1_EXT2;
    if(on)
    {
        IO_GIO_BITSET0 = (1 << 14);
        ata_reset();
    }
    else
        IO_GIO_BITCLR0 = (1 << 14);
    IO_INTC_EINT1 |= INTR_EINT1_EXT2;
	return;
#endif
}

inline bool ide_powered(){
#if 0
	return (IO_GIO_BITSET0 & (1 << 14));
#else
    return true;
#endif
}

void ata_reset(void)
{
/* Disabled until figured out what's wrong */
	IO_INTC_EINT1 &= ~INTR_EINT1_EXT2; //disable GIO2 interrupt
	if(!ide_powered())
	{
        ide_power_enable(true);
		sleep(150);
	}
	else
	{
		IO_GIO_BITSET0 = (1 << 5);
		IO_GIO_BITCLR0 = (1 << 3);
		sleep(1);
	}
	IO_GIO_BITCLR0 = (1 << 5);
	sleep(10);
	IO_GIO_BITSET0 = (1 << 3);
	while(!(ATA_COMMAND & STATUS_RDY))
		sleep(10);
	IO_INTC_EINT1 |= INTR_EINT1_EXT2; //enable GIO2 interrupt
	return;
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
    IO_INTC_EINT1 |= INTR_EINT1_EXT2;   //enable GIO2 interrupt
    //TODO: mimic OF inits...
    return;
}

void GIO2(void)
{
#ifdef DEBUG
    //printf("GIO2 interrupt...");
#endif
    IO_INTC_IRQ1 = INTR_IRQ1_EXT2; //Mask GIO2 interrupt
    return;
}

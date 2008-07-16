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
#include "jz4740.h"
#include "mipsregs.h"
#include "panic.h"

void intr_handler(void)
{
    //printf("Interrupt!");
    return;
}

void except_handler(void* stack_ptr, unsigned int cause, unsigned int epc)
{
    panicf("Exception occurred: [0x%x] at 0x%x (stack at 0x%x)", cause, epc, (unsigned int)stack_ptr);
}

void system_reboot(void)
{
    while(1);
}

void cli(void)
{
	register unsigned int t;
	t = read_c0_status();
	t &= ~1;
	write_c0_status(t);
}

unsigned int mips_get_sr(void)
{
	unsigned int t = read_c0_status();
	return t;
}

void sti(void)
{
	register unsigned int t;
	t = read_c0_status();
	t |= 1;
	t &= ~2;
	write_c0_status(t);
}

void tick_start(unsigned int interval_in_ms)
{
    (void)interval_in_ms;
}

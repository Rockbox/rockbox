/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Ulrich Pegelow
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
#include "system.h"
#include "logf.h"
#include "m5636-target.h"

void m5636_device_init(void)
{
    and_l(~0x00004000, &GPIO_INT_EN);  /* probably redundant: disable
                                          interrupt; just in case ... */
    and_l(~0x00000004, &GPIO1_OUT);    /* probably redundant: set GPIO34 low */
    or_l(  0x00000004, &GPIO1_ENABLE);   /* GPIO34 enable (see above) */
    or_l(  0x00000004, &GPIO1_FUNCTION); /* GPIO34 function (see above) */
}

/* for debugging purposes only */
void m5636_dump_regs(void)
{
    unsigned short *address;

    for (address = (unsigned short *)M5636_BASE;
         address < (unsigned short *)(M5636_BASE + 0x100);
         address++)
    {
         logf("m5636 A:%08lX D:%04lX", (uintptr_t)address,
              (uintptr_t)*address);
    }

    logf("GPIO_INT_EN    %08lX", GPIO_INT_EN);
    logf("GPIO1_OUT      %08lX", GPIO1_OUT);
    logf("GPIO1_ENABLE   %08lX", GPIO1_ENABLE);
    logf("GPIO1_FUNCTION %08lX", GPIO1_FUNCTION);
}

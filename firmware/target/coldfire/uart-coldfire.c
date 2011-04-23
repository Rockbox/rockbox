/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr & Nick Robinson
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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "serial.h"

void serial_setup (void)
{
    UCR0 = 0x30; /* Reset transmitter */
    UCSR0 = 0xdd; /* Timer mode */

    UCR0 = 0x10;  /* Reset pointer */
    UMR0 = 0x13; /* No parity, 8 bits */
    UMR0 = 0x07; /* 1 stop bit */

    UCR0 = 0x04; /* Tx enable */
}

int tx_rdy(void)
{
    if(USR0 & 0x04)
        return 1;
    else
        return 0;
}

int rx_rdy(void)
{
    /* a dummy */
    return 0;
}

void tx_writec(unsigned char c)
{
    UTB0 = c;
}


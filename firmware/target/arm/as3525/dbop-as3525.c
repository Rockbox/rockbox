/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bertrik Sikken
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
#include "as3525.h"
#include "dbop-as3525.h"

#if defined(SANSA_FUZE)
#define DBOP_PRECHARGE 0x80FF
#elif defined(SANSA_E200V2) || defined(SANSA_C200V2)
#define DBOP_PRECHARGE 0xF0FF
#endif

static short int dbop_input_value = 0;

/* read the DBOP data pins */
unsigned short dbop_read_input(void)
{
    unsigned int dbop_ctrl_old = DBOP_CTRL;
    unsigned int dbop_timpol23_old = DBOP_TIMPOL_23;

    /* make sure that the DBOP FIFO is empty */
    while ((DBOP_STAT & (1<<10)) == 0);

    /* write DBOP_DOUT to pre-charge DBOP data lines with a defined level */
    DBOP_TIMPOL_23 = 0xe007e007;    /* no strobe towards lcd */
    int delay = 10;
    while (delay--) asm volatile ("nop\n");
    DBOP_CTRL = (1 << 19) |         /* tri-state output */
                (1 << 16) |         /* enw=1 (enable write) */
                (1 << 12);          /* ow=1 (16-bit data width) */
    DBOP_DOUT = DBOP_PRECHARGE;
    while ((DBOP_STAT & (1<<10)) == 0);

#if defined(SANSA_FUZE) || defined(SANSA_E200V2)
    delay = 50;
    while (delay--) asm volatile ("nop\n");
#endif

    /* perform a DBOP read */
    DBOP_CTRL = (1 << 19) |         /* tri-state output */
                (1 << 15) |         /* strd=1 (start read) */
                (1 << 12) |         /* ow=1 (16-bit data width) */
                (31 << 0);          /* rs_t=31 (read DBOP at end of cycle) */
    while ((DBOP_STAT & (1<<16)) == 0);
    dbop_input_value = DBOP_DIN;

    /* restore previous values */
    DBOP_TIMPOL_23 = dbop_timpol23_old;
    DBOP_CTRL = dbop_ctrl_old;

    return dbop_input_value;
}

/* for the debug menu */
unsigned short dbop_debug(void)
{
    return dbop_input_value;
}

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
#include <inttypes.h>
#include "as3525.h"
#include "dbop-as3525.h"

#if defined(SANSA_FUZE) || defined(SANSA_FUZEV2)
#define DBOP_PRECHARGE 0x80FF
#elif defined(SANSA_E200V2) || defined(SANSA_C200V2)
#define DBOP_PRECHARGE 0xF0FF
#endif

#if CONFIG_CPU == AS3525
/* doesn't work with the new ams sansas so far and is not needed */
static short int dbop_input_value = 0;

#if defined(SANSA_C200V2)
/*
 * workaround DBOP noise issue cause it's really annoying if your
 * buttons don't work in the debug menu...
 */
static short int input_value_tmp[2];
int dbop_denoise_reject = 0;
int dbop_denoise_accept = 0;
#endif

/* read the DBOP data pins */
#if defined(SANSA_C200V2)
unsigned short dbop_read_input_once(void);

unsigned short dbop_read_input(void)
{
    int i;

    while (1) {
        for (i=0; i<2; i++) {
            input_value_tmp[i] = dbop_read_input_once();
        }
        /* noise rejection */
        if (input_value_tmp[0] == input_value_tmp[1]) {
            dbop_denoise_accept++;
            break;
        } else {
            dbop_denoise_reject++;
        }
    }
    if (dbop_denoise_accept + dbop_denoise_reject > 1000) {
        dbop_denoise_accept /= 2;
        dbop_denoise_reject /= 2;
    }

    return dbop_input_value;
}

unsigned short dbop_read_input_once(void)
#else
unsigned short dbop_read_input(void)
#endif
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

#endif

static inline void dbop_set_mode(int mode)
{
    int delay = 10;
    unsigned long ctrl = DBOP_CTRL;
    int curr_mode = (DBOP_CTRL >> 13) & 0x3; // bits 14:13
#ifdef SANSA_FUZEV2
    if (mode == 32 && curr_mode != 1<<1)
        DBOP_CTRL = (ctrl & ~(1<<13)) | (1<<14); // 2 serial half words
    else if (mode == 16 && curr_mode != 1<<0)
        DBOP_CTRL = (ctrl & ~(1<<14)) | (1<<13); // 2 serial bytes
#else
    if (mode == 32 && curr_mode == 0)
        DBOP_CTRL = ctrl | (1<<13|1<<14); /* 2 serial half words */
    else if (mode == 16 && curr_mode == (1<<1|1<<0))
        DBOP_CTRL =  ctrl & ~(1<<14|1<<13); /* 1 serial half word */
#endif
    else
        return;
    while(delay--) asm volatile("nop");
}

void dbop_write_data(const int16_t* p_bytes, int count)
{
    
    const int32_t *data;
    if ((intptr_t)p_bytes & 0x3 || count == 1)
    {   /* need to do a single 16bit write beforehand if the address is
         * not word aligned or count is 1, switch to 16bit mode if needed */
        dbop_set_mode(16);
        DBOP_DOUT16 = *p_bytes++;
        if (!(--count))
            return;
    }
    /* from here, 32bit transfers are save
     * set it to transfer 4*(outputwidth) units at a time,
     * if bit 12 is set it only does 2 halfwords though (we never set it)
     * switch to 32bit output if needed */
    dbop_set_mode(32);
    data = (int32_t*)p_bytes;
    while (count > 1)
    {
        DBOP_DOUT32 = *data++;
        count -= 2;

        /* Wait if push fifo is full */
        while ((DBOP_STAT & (1<<6)) != 0);
    }
    /* While push fifo is not empty */
    while ((DBOP_STAT & (1<<10)) == 0);

    /* due to the 32bit alignment requirement or uneven count,
     * we possibly need to do a 16bit transfer at the end also */
    if (count > 0)
        dbop_write_data((int16_t*)data, 1);
}

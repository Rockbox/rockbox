/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
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
#include "hwcompat.h"
#include "kernel.h"

static struct mutex adc_mtx SHAREDBSS_ATTR;

/* used in the 2nd gen ADC interrupt */
static unsigned int_data;
static int int_status = -1;

unsigned short adc_scan(int channel)
{
    unsigned short data = 0;

    (void)channel; /* there is only one */
    mutex_lock(&adc_mtx);

    if ((IPOD_HW_REVISION >> 16) == 1)
    {
        int i, j;
        unsigned pval = GPIOB_OUTPUT_VAL;

        GPIOB_OUTPUT_VAL = pval | 0x04;  /* B2 -> high */
        for (i = 32; i > 0; --i);

        GPIOB_OUTPUT_VAL = pval;         /* B2 -> low */
        for (i = 200; i > 0; --i);

        for (j = 0; j < 8; j++)
        {
            GPIOB_OUTPUT_VAL = pval | 0x02; /* B1 -> high */
            for (i = 8; i > 0; --i);

            data = (data << 1) | ((GPIOB_INPUT_VAL & 0x08) >> 3);

            GPIOB_OUTPUT_VAL = pval;     /* B1 -> low */
            for (i = 320; i > 0; --i);
        }
    }
    else if ((IPOD_HW_REVISION >> 16) == 2)
    {
        int_status = 0;
        GPIOB_INT_LEV    |= 0x04; /* high active */
        GPIOB_INT_EN     |= 0x04; /* enable interrupt */
        GPIOB_OUTPUT_VAL |= 0x0a; /* B1, B3 -> high: start conversion */

        while (int_status >= 0)
            yield();

        data = int_data & 0xff;
    }
    mutex_unlock(&adc_mtx);
    return data;
}

/* Used for 2nd gen only. Conversion can take several milliseconds there. */
void ipod_2g_adc_int(void)
{
    if (GPIOB_INPUT_VAL & 0x04)
    {
        int_data = (int_data << 1) | ((GPIOB_INPUT_VAL & 0x10) >> 4);

        GPIOB_OUTPUT_VAL &= ~0x0a; /* B1, B3 -> low */
        /* B3 needs to be set low in the first call only, but then stays low
         * anyway so no need for special handling */
    }
    else
    {
        if (++int_status > 8)
        {
            GPIOB_INT_EN &= ~0x04;
            int_status = -1;
        }
        else
            GPIOB_OUTPUT_VAL |= 0x02;  /* B1 -> high */
    }
    GPIOB_INT_LEV ^= 0x04; /* toggle interrupt level */
    GPIOB_INT_CLR  = 0x04; /* acknowledge interrupt */
}

void adc_init(void)
{
    mutex_init(&adc_mtx);
    
    GPIOB_ENABLE |= 0x1e;  /* enable B1..B4 */

    if ((IPOD_HW_REVISION >> 16) == 1)
    {
        GPIOB_OUTPUT_EN  = (GPIOB_OUTPUT_EN & ~0x08) | 0x16;
                                         /* B1, B2, B4 -> output, B3 -> input */
        GPIOB_OUTPUT_VAL = (GPIOB_OUTPUT_VAL & ~0x06) | 0x10;
                                         /* B1, B2 -> low, B4 -> high */
    }
    else if ((IPOD_HW_REVISION >> 16) == 2)
    {
        GPIOB_OUTPUT_EN   = (GPIOB_OUTPUT_EN & ~0x14) | 0x0a;
                                         /* B1, B3 -> output, B2, B4 -> input */
        GPIOB_OUTPUT_VAL &= ~0x0a;       /* B1, B3 -> low */
        while (GPIOB_INPUT_VAL & 0x04);  /* wait for B2 == 0 */
    }
}

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "x1000-codec.h"
#include "kernel.h"
#include "x1000/aic.h"

static int x1000_icodec_regcache[JZCODEC_NUM_REGS];

static void x1000_icodec_clear_regcache(void)
{
    for(int i = 0; i < JZCODEC_NUM_REGS; ++i)
        x1000_icodec_regcache[i] = -1;
}

void x1000_icodec_open(void)
{
    x1000_icodec_clear_regcache();

    /* Ingenic does not specify any timing constraints for reset,
     * let's do a 1ms delay for fun */
    jz_writef(AIC_RGADW, ICRST(1));
    mdelay(1);
    jz_writef(AIC_RGADW, ICRST(0));

    /* TODO: need specific AIC setups */
}

void x1000_icodec_close(void)
{
    /* TODO: how to close ? */
}

int x1000_icodec_read(int reg)
{
    if(x1000_icodec_regcache[reg] < 0) {
        jz_writef(AIC_RGADW, ADDR(reg));
        x1000_icodec_regcache[reg] = jz_readf(AIC_RGDATA, DATA);
    }

    return x1000_icodec_regcache[reg];
}

void x1000_icodec_write(int reg, int value)
{
    if(x1000_icodec_regcache[reg] == value)
        return;

    jz_writef(AIC_RGADW, ADDR(reg), DATA(value));
    jz_writef(AIC_RGADW, RGWR(1));
    x1000_icodec_regcache[reg] = value;

    while(jz_readf(AIC_RGADW, RGWR));

    /* FIXME: ingenic suggests reading back to verify... why??? */
}

#ifdef HAVE_X1000_ICODEC_REC
void audiohw_set_recvol(int left, int right, int type)
{
    /* TODO: implement me */
    (void)left;
    (void)right;
    (void)type;
}
#endif

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 by Cástor Muñoz
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

#include "inttypes.h"
#include "system.h"
#include "button.h"

#include "s5l8702.h"
#include "cwheel-s5l8702.h"

void cwheel_set_gpio(int cwmode)
{
    static int last_mode = -1;
    uint32_t cfg;

    if (last_mode == cwmode)
        return;

    switch (cwmode & 0xff)
    {
        default:
        case CWMODE_CTRL:  cfg = 0x00222200; break;
        case CWMODE_MAN:   cfg = 0x000f0f00; break;
        /*case CWMODE_MANTX: cfg = 0x002f2f00; break;*/
    }

    if (cwmode & CWFLAG_LOCKED) {
        /* RTS = Output Low, DOUT = Output Low */
        cfg = (cfg & 0x00f0f000) | 0x000e0e00;
    }

    if (cwmode & CWFLAG_HSPOLL) {
        /* RTS = Pull-up Input */
        PUNB(14) |= (1 << 2);
        udelay(100);
        cfg &= 0x00fff000;
    }
    else if (last_mode & CWFLAG_HSPOLL) {
        /* disable pull-up on RTS line */
        PUNB(14) &= ~(1 << 2);
        udelay(100);
    }

    PCONE = (PCONE & ~0x00ffff00) | cfg;

    last_mode = cwmode;
}

/* wait for rising/falling edge on CLK line */
bool cwheel_wait_for_edge(int edge, int tmo)
{
    int prev_level, cur_level;
    int32_t stop = USEC_TIMER + tmo;

    cur_level = edge;
    do {
        if ((int32_t)USEC_TIMER - stop >= 0)
            return 0; /* timeout */
        prev_level = cur_level;
        cur_level = (PDATE >> 3) & 1;
    }
    while (cur_level == !edge || prev_level == edge);

    return 1; /* ok */
}

#define TXRX_TMO    500
bool cwheel_tx_data(uint32_t dout)
{
    int bit;
    bool res = 0; /* err */

    dout = (dout >> 1) | 0x80000000;

    /* RTS = Low, when this signal goes low the remote controller
       generates 32 clock pulses on CLK line, allowing to the host
       controller to send the data word */
    GPIOCMD = 0xe020e;

    for (bit = 0; bit < 32; bit++) {
        if (!cwheel_wait_for_edge(0, TXRX_TMO))
            goto bye; /* timeout */
        GPIOCMD = 0xe040e | ((dout >> bit) & 1); /* write bit */
    }

    #if 0 /* not needed */
    if (!cwheel_wait_for_edge(1, TXRX_TMO >> 3))
        goto bye;  /* timeout */
    #endif

    res = 1; /* ok */

bye:
    GPIOCMD = 0xe020f; /* RTS = High */
    return res;
}

bool cwheel_rx_data(uint32_t *din, uint32_t tmo)
{
    int bit;

    if (!cwheel_wait_for_edge(0, tmo))
        return 0; /* timeout */

    *din = 0;
    for (bit = 0; bit < 32; bit++) {
        if (!cwheel_wait_for_edge(1, TXRX_TMO))
            return 0; /* timeout */
        *din |= ((PDATE >> 5) & 1) << bit;
    }

    return 1; /* ok */
}

int cwheel_get_reg(int reg)
{
    uint32_t msg = WHEEL_MSG(0, reg, WMSG_GET);
    int res = -1; /* error */
    uint32_t din;

    cwheel_set_gpio(CWMODE_MAN);

    if (cwheel_tx_data(msg))
        if (cwheel_rx_data(&din, 1024) && ((din & 0x8000ffff) == msg))
            res = (din >> 16) & 0x7fff;

    cwheel_set_gpio(CWMODE_CTRL);

    return res;
}

#if 0 /* not used */
bool cwheel_set_reg(int reg, int value)
{
    uint32_t msg = WHEEL_MSG(value, reg, WMSG_SET);
    bool res;

    cwheel_set_gpio(CWMODE_MAN);
    res = cwheel_tx_data(msg);
    cwheel_set_gpio(CWMODE_CTRL);

    return res;
}
#endif

/* this function communicates with external wheel controller
   reading/writting GPIO, disable interrupts before using it */
int read_all_buttons(int n_retry)
{
    int btn = BUTTON_NONE;

    do {
        int32_t value = cwheel_get_reg(2);
        if (value != -1) {
            if (value & 0x01)
                btn |= BUTTON_SELECT;
            if (value & 0x02)
                btn |= BUTTON_RIGHT;
            if (value & 0x04)
                btn |= BUTTON_LEFT;
            if (value & 0x08)
                btn |= BUTTON_PLAY;
            if (value & 0x10)
                btn |= BUTTON_MENU;
            break;
        }

        udelay(25000);
    }
    while (n_retry--);

    return btn;
}

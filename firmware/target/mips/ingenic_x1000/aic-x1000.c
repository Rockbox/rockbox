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

#include "system.h"
#include "aic-x1000.h"
#include "gpio-x1000.h"
#include "x1000/aic.h"
#include "x1000/cpm.h"

/* Given a rational number m/n < 1, find its representation as a continued
 * fraction [0; a1, a2, a3, ..., a_k]. At most "cnt" terms are calculated
 * and written out to "buf". Returns the number of terms written; the result
 * is complete if this value is less than "cnt", and may be incomplete if it
 * is equal to "cnt". (Note the leading zero term is not written to "buf".)
 */
static unsigned cf_derive(unsigned m, unsigned n, unsigned* buf, unsigned cnt)
{
    unsigned wrote = 0;
    unsigned a = m / n;
    while(cnt--) {
        unsigned tmp = n;
        n = m - n * a;
        if(n == 0)
            break;

        m = tmp;
        a = m / n;
        *buf++ = a;
        wrote++;
    }

    return wrote;
}

/* Given a finite continued fraction [0; buf[0], buf[1], ..., buf[count-1]],
 * calculate the rational number m/n which it represents. Returns m and n.
 * If count is zero, then m and n are undefined.
 */
static void cf_expand(const unsigned* buf, unsigned count,
                      unsigned* m, unsigned* n)
{
    if(count == 0)
        return;

    unsigned i = count - 1;
    unsigned mx = 1, nx = buf[i];
    while(i--) {
        unsigned tmp = nx;
        nx = mx + buf[i] * nx;
        mx = tmp;
    }

    *m = mx;
    *n = nx;
}

int aic_i2s_set_mclk(x1000_clk_t clksrc, unsigned fs, unsigned mult)
{
    /* get the input clock rate */
    uint32_t src_freq = clk_get(clksrc);

    /* reject invalid parameters */
    if(mult % 64 != 0)
        return -1;

    if(clksrc == X1000_EXCLK_FREQ) {
        if(mult != 0)
            return -1;

        jz_writef(AIC_I2SCR, STPBK(1));
        jz_writef(CPM_I2SCDR, CS(0), CE(0));
        REG_AIC_I2SDIV = X1000_EXCLK_FREQ / 64 / fs;
    } else {
        if(mult == 0)
            return -1;
        if(fs*mult > src_freq)
            return -1;

        /* calculate best rational approximation that fits our constraints */
        unsigned m = 0, n = 0;
        unsigned buf[16];
        unsigned cnt = cf_derive(fs*mult, src_freq, &buf[0], 16);
        do {
            cf_expand(&buf[0], cnt, &m, &n);
            cnt -= 1;
        } while(cnt > 0 && (m > 512 || n > 8192) && (n >= 2*m));

        /* wrong values */
        if(cnt == 0 || n == 0 || m == 0)
            return -1;

        jz_writef(AIC_I2SCR, STPBK(1));
        jz_writef(CPM_I2SCDR, PCS(clksrc == X1000_CLK_MPLL ? 1 : 0),
                  CS(1), CE(1), DIV_M(m), DIV_N(n));
        jz_write(CPM_I2SCDR1, REG_CPM_I2SCDR1);
        REG_AIC_I2SDIV = (mult / 64) - 1;
    }

    jz_writef(AIC_I2SCR, STPBK(0));
    return 0;
}

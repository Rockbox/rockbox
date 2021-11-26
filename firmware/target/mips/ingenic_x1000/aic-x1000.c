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
static uint32_t cf_derive(uint32_t m, uint32_t n, uint32_t* buf, uint32_t cnt)
{
    uint32_t wrote = 0;
    uint32_t a = m / n;
    while(cnt--) {
        uint32_t tmp = n;
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
static void cf_expand(const uint32_t* buf, uint32_t count,
                      uint32_t* m, uint32_t* n)
{
    if(count == 0)
        return;

    uint32_t i = count - 1;
    uint32_t mx = 1, nx = buf[i];
    while(i--) {
        uint32_t tmp = nx;
        nx = mx + buf[i] * nx;
        mx = tmp;
    }

    *m = mx;
    *n = nx;
}

static int calc_i2s_clock_params(x1000_clk_t clksrc,
                                 uint32_t fs, uint32_t mult,
                                 uint32_t* div_m, uint32_t* div_n,
                                 uint32_t* i2sdiv)
{
    if(clksrc == X1000_CLK_EXCLK) {
        /* EXCLK mode bypasses the CPM clock so it's more limited */
        *div_m = 0;
        *div_n = 0;
        *i2sdiv = X1000_EXCLK_FREQ / 64 / fs;

        /* clamp to maximum value */
        if(*i2sdiv > 512)
            *i2sdiv = 512;
        if(*i2sdiv == 0)
            *i2sdiv = 1;

        return 0;
    }

    /* ensure a valid clock was selected */
    if(clksrc != X1000_CLK_SCLK_A &&
       clksrc != X1000_CLK_MPLL)
        return -1;

    /* ensure bit clock constraint is respected */
    if(mult % 64 != 0)
        return -1;

    /* ensure master clock frequency is not too high */
    if(fs > UINT32_MAX/mult)
        return -1;

    /* get frequencies */
    uint32_t tgt_freq = fs * mult;
    uint32_t src_freq = clk_get(clksrc);

    /* calculate best rational approximation fitting hardware constraints */
    uint32_t m = 0, n = 0;
    uint32_t buf[16];
    uint32_t cnt = cf_derive(tgt_freq, src_freq, &buf[0], 16);
    do {
        cf_expand(&buf[0], cnt, &m, &n);
        cnt -= 1;
    } while(cnt > 0 && (m > 512 || n > 8192) && (n >= 2*m));

    /* unrepresentable */
    if(cnt == 0 || n == 0 || m == 0)
        return -1;

    *div_m = m;
    *div_n = n;
    *i2sdiv = mult / 64;
    return 0;
}

uint32_t aic_calc_i2s_clock(x1000_clk_t clksrc, uint32_t fs, uint32_t mult)
{
    uint32_t m, n, i2sdiv;
    if(calc_i2s_clock_params(clksrc, fs, mult, &m, &n, &i2sdiv))
        return 0;

    unsigned long long rate = clk_get(clksrc);
    rate *= m;
    rate /= n * i2sdiv; /* this multiply can't overflow. */

    /* clamp */
    if(rate > 0xffffffffull)
        rate = 0xffffffff;

    return rate;
}

int aic_set_i2s_clock(x1000_clk_t clksrc, uint32_t fs, uint32_t mult)
{
    uint32_t m, n, i2sdiv;
    if(calc_i2s_clock_params(clksrc, fs, mult, &m, &n, &i2sdiv))
        return -1;

    /* turn off bit clock */
    bool bitclock_en = !jz_readf(AIC_I2SCR, STPBK);
    jz_writef(AIC_I2SCR, STPBK(1));

    /* handle master clock */
    if(clksrc == X1000_CLK_EXCLK) {
        jz_writef(CPM_I2SCDR, CS(0), CE(0));
    } else {
        jz_writef(CPM_I2SCDR, PCS(clksrc == X1000_CLK_MPLL ? 1 : 0),
                  CS(1), CE(1), DIV_M(m), DIV_N(n));
        jz_write(CPM_I2SCDR1, REG_CPM_I2SCDR1);
    }

    /* set bit clock divider */
    REG_AIC_I2SDIV = i2sdiv - 1;

    /* re-enable the bit clock */
    if(bitclock_en)
        jz_writef(AIC_I2SCR, STPBK(0));

    return 0;
}

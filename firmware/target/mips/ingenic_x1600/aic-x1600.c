/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Michael McAllister
 *
 * Based on firmware/target/mips/ingenic_x1000/aic-x1000.c,
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

/* EPLL -> I2S1CDR M/N -> MCLK -> I2SDIV.TDIV -> BCLK -> /64 -> LRCLK */

#include "system.h"
#include "aic-x1600.h"
#include "clk-x1600.h"
#include "x1600/aic.h"
#include "x1600/cpm.h"

int aic_set_i2s_clock(x1600_clk_t clksrc, uint32_t fs, uint32_t mult)
{
    /* EPLL is the only usable parent */
    if(clksrc != X1600_CLK_EPLL)
        return -1;

    /* BCLK is hardwired to 64 fs, so MCLK/fs must be a whole number of them */
    if(mult == 0 || (mult % AIC_I2S_BCLK_MULT) != 0)
        return -1;

    uint32_t tdiv = mult / AIC_I2S_BCLK_MULT;

    /* TDIV must be even; the hardware floors odd values an octave off */
    if(tdiv == 0 || (tdiv & 1) != 0 || tdiv > BM_AIC_I2SDIV_TDIV)
        return -1;

    /* Disable the bit clock while changing its divider. */
    bool bitclock_was_on = aic_i2s_bit_clock_enabled();
    aic_enable_i2s_bit_clock(false);

    /* TDIV, not the X1000's TDIV-1; read-modify-write leaves RDIV alone */
    jz_writef(AIC_I2SDIV, TDIV(tdiv));

    /* Re-asserts CE_I2S, hence restoring the gate below */
    uint32_t mclk = clk_set_i2s_mclk(fs, mult);
    if(mclk == 0) {
        aic_enable_i2s_bit_clock(bitclock_was_on);
        return -1;
    }

    aic_enable_i2s_bit_clock(bitclock_was_on);
    return 0;
}

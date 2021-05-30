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

#ifndef __AIC_X1000_H__
#define __AIC_X1000_H__

#include "clk-x1000.h"
#include "x1000/aic.h"
#include <stdbool.h>
#include <stdint.h>

#define AIC_I2S_MASTER_MODE       0
#define AIC_I2S_MASTER_EXCLK_MODE 1
#define AIC_I2S_SLAVE_MODE        2

#define AIC_I2S_LEFT_CHANNEL_FIRST  0
#define AIC_I2S_RIGHT_CHANNEL_FIRST 1

/* Nb. the functions below are intended to serve as "documentation" and make
 * target audiohw code clearer, they should normally be called with immediate
 * constant arguments so they are inlined to a register read-modify-write. */

/* Enable/disable some kind of big-endian mode. Presumably it refers to
 * the endianness of the samples read or written to the FIFO. */
static inline void aic_set_big_endian_format(bool en)
{
    jz_writef(AIC_CFG, MSB(en ? 1 : 0));
}

/* Set whether to send the last sample (true) or a zero sample (false)
 * if the AIC FIFO underflows during playback. */
static inline void aic_set_play_last_sample(bool en)
{
    jz_writef(AIC_CFG, LSMP(en ? 1 : 0));
}

/* Select the use of the internal or external codec. */
static inline void aic_set_external_codec(bool en)
{
    jz_writef(AIC_CFG, ICDC(en ? 0 : 1));
}

/* Set I2S interface mode */
static inline void aic_set_i2s_mode(int mode)
{
    switch(mode) {
    default:
    case AIC_I2S_MASTER_MODE:
        jz_writef(AIC_CFG, BCKD(1), SYNCD(1));
        break;

    case AIC_I2S_MASTER_EXCLK_MODE:
        jz_writef(AIC_CFG, BCKD(0), SYNCD(1));
        break;

    case AIC_I2S_SLAVE_MODE:
        jz_writef(AIC_CFG, BCKD(0), SYNCD(0));
        break;
    }
}

/* Select the channel ordering on the I2S interface (playback only). */
static inline void aic_set_i2s_channel_order(int order)
{
    switch(order) {
    default:
    case AIC_I2S_LEFT_CHANNEL_FIRST:
        jz_writef(AIC_I2SCR, RFIRST(0));
        break;

    case AIC_I2S_RIGHT_CHANNEL_FIRST:
        jz_writef(AIC_I2SCR, RFIRST(1));
        break;
    }
}

/* Enable/disable the I2S master clock (also called 'system clock') */
static inline void aic_enable_i2s_master_clock(bool en)
{
    jz_writef(AIC_I2SCR, ESCLK(en ? 1 : 0));
}

/* Enable/disable the I2S bit clock */
static inline void aic_enable_i2s_bit_clock(bool en)
{
    jz_writef(AIC_I2SCR, STPBK(en ? 0 : 1));
}

/* Select whether I2S mode is used (false) or MSB-justified mode (true). */
static inline void aic_set_msb_justified_mode(bool en)
{
    jz_writef(AIC_I2SCR, AMSL(en ? 1 : 0));
}

/* Calculate frequency of I2S clocks.
 *
 * - 'clksrc' can be one of EXCLK, SCLK_A, or MPLL.
 * - 'fs' is the audio sampling frequency in Hz, must be 8 KHz - 192 KHz.
 * - The master clock frequency equals 'mult * fs' Hz. Due to hardware
 *   restrictions, 'mult' must be divisible by 64.
 *
 * - NOTE: When using EXCLK source, the master clock equals EXCLK and the
 *   'mult' parameter is ignored.
 *
 * This function returns the actual bit clock rate which would be achieved.
 * (Note the bit clock is always 64x the effective sampling rate.)
 *
 * If the exact rate cannot be attained, then this will return the closest
 * possible rate to the desired rate. In case of invalid parameters, this
 * function will return zero. That also occurs if the chosen PLL is stopped.
 */
extern uint32_t aic_calc_i2s_clock(x1000_clk_t clksrc,
                                   uint32_t fs, uint32_t mult);

/* Set the I2S clock frequency.
 *
 * Parameters are the same as 'aic_calc_i2s_clock()' except this function
 * will set the clocks. If the bit clock is running, it will be automatically
 * stopped and restarted properly.
 *
 * Returns zero on success. If an invalid state occurs (due to bad settings)
 * then this function will do nothing and return a nonzero value.
 */
extern int aic_set_i2s_clock(x1000_clk_t clksrc, uint32_t fs, uint32_t mult);

#endif /* __AIC_X1000_H__ */

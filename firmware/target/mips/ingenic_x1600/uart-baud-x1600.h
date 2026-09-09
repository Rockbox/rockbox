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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __UART_BAUD_X1600_H__
#define __UART_BAUD_X1600_H__

#include <stdint.h>

/* Search UMR and UDLHR:UDLLR for baud = clock / (M * divisor).
 * Requires 0 < baud <= clk, with clk = EXCLK (24 MHz). */
#define UART_M_MIN  4
#define UART_M_MAX  31

static inline void uart_calc_divisor(uint32_t clk, int baud,
                                     uint32_t *out_m, uint32_t *out_dl,
                                     uint32_t *out_rate, uint32_t *out_err)
{
    uint32_t best_m = 0, best_dl = 0, best_rate = 0;
    uint32_t best_err = 0xffffffff;

    for(uint32_t m = UART_M_MIN; m <= UART_M_MAX; ++m) {
        /* Nearest divisor for this M */
        uint32_t dl = (clk + (uint32_t)baud * m / 2) / ((uint32_t)baud * m);
        if(dl == 0 || dl > 0xffff)
            continue;

        uint32_t rate = clk / (m * dl);
        uint32_t err = rate > (uint32_t)baud ? rate - baud : baud - rate;

        /* On equal error, prefer the conventional oversampling factor of
         * 16. */
        uint32_t d16      = m > 16 ? m - 16 : 16 - m;
        uint32_t best_d16 = best_m > 16 ? best_m - 16 : 16 - best_m;
        if(err < best_err || (err == best_err && d16 < best_d16)) {
            best_err = err; best_m = m; best_dl = dl; best_rate = rate;
        }
    }

    *out_m = best_m; *out_dl = best_dl;
    *out_rate = best_rate; *out_err = best_err;
}

#endif /* __UART_BAUD_X1600_H__ */

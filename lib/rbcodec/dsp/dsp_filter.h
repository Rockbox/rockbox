/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Thom Johansen
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
#ifndef DSP_FILTER_H
#define DSP_FILTER_H

/** Basic filter implementations which may be used independently **/

/* Used by: EQ, tone controls and crossfeed */

/* These depend on the fixed point formats used by the different filter types
   and need to be changed when they change.
 */
struct dsp_filter
{
    int32_t coefs[5];      /* 00h: Order is b0, b1, b2, a1, a2 */
    int32_t history[2][4]; /* 14h: Order is x-1, x-2, y-1, y-2, per channel */
    uint8_t shift;         /* 34h: Final shift after computation */
                           /* 38h */
};

void filter_shelf_coefs(unsigned long cutoff, long A, bool low, int32_t *c);
#ifdef HAVE_SW_TONE_CONTROLS
void filter_bishelf_coefs(unsigned long cutoff_low,
                          unsigned long cutoff_high,
                          long A_low, long A_high, long A,
                          struct dsp_filter *f);
#endif /* HAVE_SW_TONE_CONTROLS */
void filter_pk_coefs(unsigned long cutoff, unsigned long Q, long db,
                     struct dsp_filter *f);
void filter_ls_coefs(unsigned long cutoff, unsigned long Q, long db,
                     struct dsp_filter *f);
void filter_hs_coefs(unsigned long cutoff, unsigned long Q, long db,
                     struct dsp_filter *f);
void filter_copy(struct dsp_filter *dst, const struct dsp_filter *src);
void filter_flush(struct dsp_filter *f);
void filter_process(struct dsp_filter *f, int32_t * const buf[], int count,
                    unsigned int channels);

#endif /* DSP_FILTER_H */

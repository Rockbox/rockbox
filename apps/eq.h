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

#ifndef _EQ_H
#define _EQ_H

#include <inttypes.h>

/* These depend on the fixed point formats used by the different filter types
   and need to be changed when they change.
 */
#define FILTER_BISHELF_SHIFT 5
#define EQ_PEAK_SHIFT 4
#define EQ_SHELF_SHIFT 6

struct eqfilter {
    int32_t coefs[5];        /* Order is b0, b1, b2, a1, a2 */
    int32_t history[2][4];
};

void filter_shelf_coefs(unsigned long cutoff, long A, bool low, int32_t *c);
void filter_bishelf_coefs(unsigned long cutoff_low, unsigned long cutoff_high,
                          long A_low, long A_high, long A, int32_t *c);
void eq_pk_coefs(unsigned long cutoff, unsigned long Q, long db, int32_t *c);
void eq_ls_coefs(unsigned long cutoff, unsigned long Q, long db, int32_t *c);
void eq_hs_coefs(unsigned long cutoff, unsigned long Q, long db, int32_t *c);
void eq_filter(int32_t **x, struct eqfilter *f, unsigned num,
               unsigned channels, unsigned shift);

#endif


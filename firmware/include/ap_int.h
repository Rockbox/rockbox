/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2018 by Michael A. Sevakis
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
#ifndef AP_INT_H
#define AP_INT_H

/* Miscellaneous large-sized integer functions */

#include <stdbool.h>
#include <stdint.h>

/* return floor(log(2)*base2exp) - assists in estimating buffer sizes
 * when converting to decimal */
static inline int base10exp(int base2exp)
{
    /* 1292913986 = floor(2^32*log(2)) */
    static const long log10of2 = 1292913986L;
    return log10of2 * (int64_t)base2exp >> 32;
}

struct ap_int
{
    long     numchunks; /* number of uint32_t chunks or zero */
    long     basechunk; /* chunk of start of value bits */
    uint32_t *chunks;    /* pointer to chunk array (caller alloced) */
    long     len;        /* length of output */
    long     shift;      /* number of fractional bits */
    uint64_t val;        /* value, if it fits and numchunks is zero */
};

bool round_number_string10(char *p_rdig, long len);

/* format arbitrary-precision base 10 integer */
char * format_ap_int10(struct ap_int *a,
                       char *p_end);

/* format arbitrary-precision base 10 fraction */
char * format_ap_frac10(struct ap_int *a,
                        char *p_start,
                        long precision);

#endif /* AP_INT_H */

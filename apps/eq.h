/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Thom Johansen
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _EQ_H
#define _EQ_H

/* These depend on the fixed point formats used by the different filter types
   and need to be changed when they change.
 */
#define EQ_PEAK_SHIFT 3
#define EQ_SHELF_SHIFT 7

struct eqfilter {
    long coefs[5];        /* Order is b0, b1, b2, a1, a2 */
    long history[2][4];
};

void eq_pk_coefs(unsigned long cutoff, unsigned long Q, long db, long *c);
void eq_ls_coefs(unsigned long cutoff, unsigned long Q, long db, long *c);
void eq_hs_coefs(unsigned long cutoff, unsigned long Q, long db, long *c);
void eq_filter(long **x, struct eqfilter *f, unsigned num,
               unsigned samples, unsigned shift);

#endif


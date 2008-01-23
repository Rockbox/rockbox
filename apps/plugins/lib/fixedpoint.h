/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jens Arnold
 *
 * Fixed point library for plugins
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

long fsincos(unsigned long phase, long *cos);
long fsqrt(long a, unsigned int fracbits);
long cos_int(int val);
long sin_int(int val);
long flog(int x);

/* fast unsigned multiplication (16x16bit->32bit or 32x32bit->32bit,
 * whichever is faster for the architecture) */
#ifdef CPU_ARM
#define FMULU(a, b) ((uint32_t) (((uint32_t) (a)) * ((uint32_t) (b))))
#else /* SH1, coldfire */
#define FMULU(a, b) ((uint32_t) (((uint16_t) (a)) * ((uint16_t) (b))))
#endif

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Christian Gmeiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __MCF5250_H__
#define __MCF5250_H__

#include "mcf5249.h"

/* here we undefine stuff which is different from mcf5249 */
#undef SPURVEC
#undef INTBASE
#undef TER0
#undef TER1

/* here we remove stuff, which is not included in mfc5250 */
#undef DACR1
#undef DMR1
#undef INTERRUPTSTAT3
#undef INTERRUPTCLEAR3
#undef INTERRUPTEN3
#undef IPERRORADR

/* here we define some new stuff */

#define CSAR4 (*(volatile unsigned long *)(MBAR + 0x0b0))   /* Chip Select Address Register Bank 4  */
#define CSMR4 (*(volatile unsigned long *)(MBAR + 0x0b4))   /* Chip Select Mask Register Bank 4     */
#define CSCR4 (*(volatile unsigned long *)(MBAR + 0x0b8))   /* Chip Select Control Register Bank 4  */

/* here we define changed stuff */
#define TER0  (*(volatile unsigned short *)(MBAR + 0x151))  /* Timer0 Event Register */
#define TER1  (*(volatile unsigned short *)(MBAR + 0x191))  /* Timer1 Event Register */

#define SPURVEC  (*(volatile unsigned char *)(MBAR2 + 0x164))   /* spurious secondary interrupt vector      */
#define INTBASE  (*(volatile unsigned char *)(MBAR2 + 0x168))   /* secondary interrupt base vector register */

#endif

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
#define IPR    (*(volatile unsigned long *)(MBAR + 0x040))  /* interrupt oending register    */
#define IMR    (*(volatile unsigned long *)(MBAR + 0x044))  /* Interrupt Mask Register       */

#define ICR1   (*(volatile unsigned long *)(MBAR + 0x04d))  /* Primary interrupt control reg: timer 0   */
#define ICR2   (*(volatile unsigned long *)(MBAR + 0x04e))  /* Primary interrupt control reg: timer 1   */
#define ICR3   (*(volatile unsigned long *)(MBAR + 0x04f))  /* Primary interrupt control reg: i2c0      */
#define ICR5   (*(volatile unsigned long *)(MBAR + 0x051))  /* Primary interrupt control reg: uart1     */
#define ICR6   (*(volatile unsigned long *)(MBAR + 0x052))  /* Primary interrupt control reg: dma0      */
#define ICR7   (*(volatile unsigned long *)(MBAR + 0x053))  /* Primary interrupt control reg: dam1      */
#define ICR9   (*(volatile unsigned long *)(MBAR + 0x055))  /* Primary interrupt control reg: dam3      */
#define ICR10  (*(volatile unsigned long *)(MBAR + 0x056))  /* Primary interrupt control reg: qspi      */

#define CSAR4 (*(volatile unsigned long *)(MBAR + 0x0b0))   /* Chip Select Address Register Bank 4  */
#define CSMR4 (*(volatile unsigned long *)(MBAR + 0x0b4))   /* Chip Select Mask Register Bank 4     */
#define CSCR4 (*(volatile unsigned long *)(MBAR + 0x0b8))   /* Chip Select Control Register Bank 4  */

/* here we define changed stuff */
#define TER0  (*(volatile unsigned short *)(MBAR + 0x151))  /* Timer0 Event Register */
#define TER1  (*(volatile unsigned short *)(MBAR + 0x191))  /* Timer1 Event Register */

#define SPURVEC  (*(volatile unsigned char *)(MBAR2 + 0x164))   /* spurious secondary interrupt vector      */
#define INTBASE  (*(volatile unsigned char *)(MBAR2 + 0x168))   /* secondary interrupt base vector register */

#endif

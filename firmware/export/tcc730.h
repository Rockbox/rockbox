/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Jean-Philippe Bernardy
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef TCC730_H
#define TCC730_H

#include "types.h"

/* int is 16 bit 
   long is 32 bit */

#define IOBASE (0x3f0000)
#define MMIO(t, x) (*(volatile t*)(IOBASE+(x))) 

#define WDTCON MMIO(unsigned char, 0x07)

#define TACON  MMIO(unsigned char, 0x08)
#define TAPRE  MMIO(unsigned char, 0x09)
#define TADATA MMIO(unsigned int, 0x0A)
#define TACNT MMIO(unsigned int, 0x0C)

#define IMR0 MMIO(unsigned int, 0x22)
#define IMR1 MMIO(unsigned int, 0x2A)

#define P0 MMIO(unsigned char, 0x30)
#define P1 MMIO(unsigned char, 0x31)
#define P2 MMIO(unsigned char, 0x32)

#define ADDATA MMIO(unsigned int, 0x74)
#define ADCON MMIO(unsigned char, 0x76)

#define PLL0DATA MMIO(unsigned char, 0xA8)
#define PLL0CON MMIO(unsigned int, 0xAA)
#define PLL1DATA MMIO(unsigned char, 0xAC)
#define PLL1CON MMIO(unsigned int, 0xAE)

#define MIUSCFG MMIO(unsigned char, 0x110)

#define DDMACOM MMIO(unsigned char, 0x120)
#define DDMACFG MMIO(unsigned char, 0x121)
#define DDMAIADR MMIO(unsigned long, 0x122)
#define DDMAEADR MMIO(unsigned long, 0x126)
#define DDMANUM MMIO(unsigned int, 0x12A)
#define DDMACNT MMIO(unsigned int, 0x12C)

#endif

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

/* int is 16 bit 
   long is 32 bit */

#define IOBASE (0x3f0000)
#define MMIO(t, x) (*(volatile t*)(IOBASE+(x))) 

#define WDTEN MMIO(unsigned char, 0x06)
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
#define P3 MMIO(unsigned char, 0x33)
#define P4 MMIO(unsigned char, 0x34)
#define P5 MMIO(unsigned char, 0x35)
#define P6 MMIO(unsigned char, 0x36)
#define P7 MMIO(unsigned char, 0x37)
#define P8 MMIO(unsigned char, 0x38)
#define P9 MMIO(unsigned char, 0x39)
#define P10 MMIO(unsigned char, 0x3A)

#define P0CON  MMIO(unsigned char,0x40)
#define P1CON  MMIO(unsigned char,0x41)
#define P2CON  MMIO(unsigned int,0x42)
#define P2CONH MMIO(unsigned char,0x42)
#define P2CONL MMIO(unsigned char,0x43)
#define P3CON  MMIO(unsigned int,0x44)
#define P3CONH MMIO(unsigned char,0x44)
#define P3CONL MMIO(unsigned char,0x45)
#define P3PUR  MMIO(unsigned char,0x46)
#define P5CON              MMIO(unsigned char,0x48)
#define P5PUR              MMIO(unsigned char,0x49)
#define P5INTMOD           MMIO(unsigned int,0x4A)
#define P5INTCON           MMIO(unsigned char,0x4C)
#define P4CON              MMIO(unsigned char,0x50)
#define P4INTCON           MMIO(unsigned char,0x51)
#define P4INTMOD           MMIO(unsigned char,0x52)
#define P6CON              MMIO(unsigned char,0x53)
#define P7CON              MMIO(unsigned char,0x54)
#define P8CON              MMIO(unsigned char,0x55)
#define P9CON              MMIO(unsigned char,0x56)
#define P10CON             MMIO(unsigned char,0x57)

#define ADDATA MMIO(unsigned int, 0x74)
#define ADCON MMIO(unsigned char, 0x76)

#define PLL0DATA MMIO(unsigned int, 0xA8)
#define PLL0CON MMIO(unsigned char, 0xAA)
#define PLL1DATA MMIO(unsigned int, 0xAC)
#define PLL1CON MMIO(unsigned char, 0xAE)

#define MIUSCFG MMIO(unsigned char, 0x110)
#define MIUDCOM MMIO(unsigned char, 0x111)
#define MIUDCFG MMIO(unsigned int, 0x112)
#define MIUDCNT MMIO(unsigned int, 0x114)



#define DDMACOM MMIO(unsigned char, 0x120)
#define DDMACFG MMIO(unsigned char, 0x121)
#define DDMAIADR MMIO(unsigned long, 0x122)
#define DDMAEADR MMIO(unsigned long, 0x126)
#define DDMANUM MMIO(unsigned int, 0x12A)
#define DDMACNT MMIO(unsigned int, 0x12C)

#endif

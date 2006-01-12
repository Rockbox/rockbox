/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Tomasz Malesinski
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __PNX0101_H__
#define __PNX0101_H__

#define GPIO0_READ (*(volatile unsigned long *)0x80003000)
#define GPIO0_SET  (*(volatile unsigned long *)0x80003014)
#define GPIO0_CLR  (*(volatile unsigned long *)0x80003018)
#define GPIO1_READ (*(volatile unsigned long *)0x80003040)
#define GPIO1_SET  (*(volatile unsigned long *)0x80003054)
#define GPIO1_CLR  (*(volatile unsigned long *)0x80003058)
#define GPIO2_READ (*(volatile unsigned long *)0x80003080)
#define GPIO2_SET  (*(volatile unsigned long *)0x80003094)
#define GPIO2_CLR  (*(volatile unsigned long *)0x80003098)
#define GPIO3_READ (*(volatile unsigned long *)0x800030c0)
#define GPIO3_SET  (*(volatile unsigned long *)0x800030d4)
#define GPIO3_CLR  (*(volatile unsigned long *)0x800030d8)
#define GPIO4_READ (*(volatile unsigned long *)0x80003100)
#define GPIO4_SET  (*(volatile unsigned long *)0x80003114)
#define GPIO4_CLR  (*(volatile unsigned long *)0x80003118)
#define GPIO5_READ (*(volatile unsigned long *)0x80003140)
#define GPIO5_SET  (*(volatile unsigned long *)0x80003154)
#define GPIO5_CLR  (*(volatile unsigned long *)0x80003158)
#define GPIO6_READ (*(volatile unsigned long *)0x80003180)
#define GPIO6_SET  (*(volatile unsigned long *)0x80003194)
#define GPIO6_CLR  (*(volatile unsigned long *)0x80003198)
#define GPIO7_READ (*(volatile unsigned long *)0x800031c0)
#define GPIO7_SET  (*(volatile unsigned long *)0x800031d4)
#define GPIO7_CLR  (*(volatile unsigned long *)0x800031d8)

#define LCDREG04 (*(volatile unsigned long *)0x80104004)
#define LCDSTAT  (*(volatile unsigned long *)0x80104008)
#define LCDREG10 (*(volatile unsigned long *)0x80104010)
#define LCDCMD   (*(volatile unsigned long *)0x80104020)
#define LCDDATA  (*(volatile unsigned long *)0x80104030)

#define TIMERR00 (*(volatile unsigned long *)0x80020000)
#define TIMERR08 (*(volatile unsigned long *)0x80020008)
#define TIMERR0C (*(volatile unsigned long *)0x8002000c)

#define ADCCH0   (*(volatile unsigned long *)0x80002400)
#define ADCCH1   (*(volatile unsigned long *)0x80002404)
#define ADCCH2   (*(volatile unsigned long *)0x80002408)
#define ADCCH3   (*(volatile unsigned long *)0x8000240c)
#define ADCCH4   (*(volatile unsigned long *)0x80002410)
#define ADCST    (*(volatile unsigned long *)0x80002420)
#define ADCR24   (*(volatile unsigned long *)0x80002424)
#define ADCR28   (*(volatile unsigned long *)0x80002428)

#endif

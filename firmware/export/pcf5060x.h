/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Rob Purchase
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef PCF5060X_H
#define PCF5060X_H

/* PCF50605/6 Registers from datasheet rev2.2 */

#define PCF5060X_ID      0x00
#define PCF5060X_OOCS    0x01
#define PCF5060X_INT1    0x02
#define PCF5060X_INT2    0x03
#define PCF5060X_INT3    0x04
#define PCF5060X_INT1M   0x05
#define PCF5060X_INT2M   0x06
#define PCF5060X_INT3M   0x07
#define PCF5060X_OOCC1   0x08
  #define GOSTDBY  0x1
  #define TOTRST   (0x1 << 1)
  #define CLK32ON  (0x1 << 2)
  #define WDTRST   (0x1 << 3)
  #define RTCWAK   (0x1 << 4)
  #define CHGWAK   (0x1 << 5)
  #define EXTONWAK (0x01 << 6)
#define PCF5060X_OOCC2   0x09
#define PCF5060X_RTCSC   0x0a
#define PCF5060X_RTCMN   0x0b
#define PCF5060X_RTCHR   0x0c
#define PCF5060X_RTCWD   0x0d
#define PCF5060X_RTCDT   0x0e
#define PCF5060X_RTCMT   0x0f
#define PCF5060X_RTCYR   0x10
#define PCF5060X_RTCSCA  0x11
#define PCF5060X_RTCMNA  0x12
#define PCF5060X_RTCHRA  0x13
#define PCF5060X_RTCWDA  0x14
#define PCF5060X_RTCDTA  0x15
#define PCF5060X_RTCMTA  0x16
#define PCF5060X_RTCYRA  0x17
#define PCF5060X_PSSC    0x18
#define PCF5060X_PWROKM  0x19
#define PCF5060X_PWROKS  0x1a
#define PCF5060X_DCDC1   0x1b
#define PCF5060X_DCDC2   0x1c
#define PCF5060X_DCDC3   0x1d
#define PCF5060X_DCDC4   0x1e
#define PCF5060X_DCDEC1  0x1f
#define PCF5060X_DCDEC2  0x20
#define PCF5060X_DCUDC1  0x21
#define PCF5060X_DCUDC2  0x22
#define PCF5060X_IOREGC  0x23
#define PCF5060X_D1REGC1 0x24
#define PCF5060X_D2REGC1 0x25
#define PCF5060X_D3REGC1 0x26
#define PCF5060X_LPREGC1 0x27
#define PCF5060X_LPREGC2 0x28
#define PCF5060X_MBCC1   0x29
#define PCF5060X_MBCC2   0x2a
#define PCF5060X_MBCC3   0x2b
#define PCF5060X_MBCS1   0x2c
#define PCF5060X_BBCC    0x2d
#define PCF5060X_ADCC1   0x2e
#define PCF5060X_ADCC2   0x2f
#define PCF5060X_ADCS1   0x30
#define PCF5060X_ADCS2   0x31
#define PCF5060X_ADCS3   0x32
#define PCF5060X_ACDC1   0x33
#define PCF5060X_BVMC    0x34
#define PCF5060X_PWMC1   0x35
#define PCF5060X_LEDC1   0x36
#define PCF5060X_LEDC2   0x37
#define PCF5060X_GPOC1   0x38
#define PCF5060X_GPOC2   0x39
#define PCF5060X_GPOC3   0x3a
#define PCF5060X_GPOC4   0x3b
#define PCF5060X_GPOC5   0x3c

#endif /* PCF5060X_H */

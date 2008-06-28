/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Ulrich Pegelow
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
#ifndef M5636_TARGET_H
#define M5636_TARGET_H

#define M5636_BASE 0xf0004000L /* Rockbox: 0xf0004000; OF: 0x20004000 */

/* We are currently lacking a datasheet for the M5636. No mnemonics available.
   The registers are named according to their respective hexadecimal offsets.
*/
   
#define M5636_4064 (*(volatile unsigned short *)(M5636_BASE + 0x64L))
#define M5636_4068 (*(volatile unsigned short *)(M5636_BASE + 0x68L))
#define M5636_4078 (*(volatile unsigned short *)(M5636_BASE + 0x78L))

extern void m5636_device_init(void);

/* for debugging purposes only */
extern void m5636_dump_regs(void);

#endif

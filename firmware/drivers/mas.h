/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _MAS_H_
#define _MAS_H_

#define MAS_BANK_D0 0
#define MAS_BANK_D1 1

/*
	MAS I2C	defs
*/
#define MAS_ADR         0x3a
#define	MAS_DEV_WRITE   (MAS_ADR | 0x00)
#define	MAS_DEV_READ    (MAS_ADR | 0x01)

/* registers..*/
#define	MAS_DATA_WRITE  0x68
#define MAS_DATA_READ   0x69
#define	MAS_CONTROL     0x6a

/*
 *	MAS register
 */
#define	MAS_REG_DCCF            0x8e
#define	MAS_REG_MUTE            0xaa
#define	MAS_REG_PIODATA         0xc8
#define	MAS_REG_StartUpConfig   0xe6
#define	MAS_REG_KPRESCALE       0xe7
#define	MAS_REG_KBASS           0x6b
#define	MAS_REG_KTREBLE         0x6f

int mas_run(int prognum);
int mas_readmem(int bank, int addr, unsigned long* dest, int len);
int mas_writemem(int bank, int addr, unsigned long* src, int len);
int mas_devread(unsigned long *buf, int len);
int mas_readreg(int reg);
int mas_writereg(int reg, unsigned short val);

#endif

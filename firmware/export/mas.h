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

#define MAX_PEAK 0x8000

/*
	MAS I2C	defs
*/
#if CONFIG_HWCODEC == MAS3587F
#define MAS_ADR         0x3c
#define	MAS_DEV_WRITE   (MAS_ADR | 0x00)
#define	MAS_DEV_READ    (MAS_ADR | 0x01)
#else
#define MAS_ADR         0x3a
#define	MAS_DEV_WRITE   (MAS_ADR | 0x00)
#define	MAS_DEV_READ    (MAS_ADR | 0x01)
#endif

/* registers..*/
#if CONFIG_HWCODEC == MAS3587F
#define	MAS_DATA_WRITE  0x68
#define MAS_DATA_READ   0x69
#define	MAS_CODEC_WRITE  0x6c
#define MAS_CODEC_READ   0x6d
#define	MAS_CONTROL     0x6a
#define	MAS_DCCF        0x76
#define	MAS_DCFR        0x77
#else
#define	MAS_DATA_WRITE  0x68
#define MAS_DATA_READ   0x69
#define	MAS_CONTROL     0x6a
#endif

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
#if CONFIG_HWCODEC == MAS3587F
#define MAS_REG_KMDB_SWITCH     0x21
#define MAS_REG_KMDB_STR        0x22
#define MAS_REG_KMDB_HAR        0x23
#define MAS_REG_KMDB_FC         0x24
#define MAS_REG_KLOUDNESS       0x1e
#define MAS_REG_QPEAK_L         0x0a
#define MAS_REG_QPEAK_R   	 0x0b
#define MAS_REG_DQPEAK_L	 0x0c
#define MAS_REG_DQPEAK_R        0x0d
#define MAS_REG_KAVC            0x12
#endif

/*
 * MAS commands
 */
#if CONFIG_HWCODEC == MAS3587F
#define MAS_CMD_READ_ANCILLARY  0x50
#define MAS_CMD_FAST_PRG_DL     0x60
#define MAS_CMD_READ_IC_VER     0x70
#define MAS_CMD_READ_REG        0xa0
#define MAS_CMD_WRITE_REG       0xb0
#define MAS_CMD_READ_D0_MEM     0xc0
#define MAS_CMD_READ_D1_MEM     0xd0
#define MAS_CMD_WRITE_D0_MEM    0xe0
#define MAS_CMD_WRITE_D1_MEM    0xf0
#else
#define MAS_CMD_READ_ANCILLARY  0x30
#define MAS_CMD_WRITE_REG       0x90
#define MAS_CMD_WRITE_D0_MEM    0xa0
#define MAS_CMD_WRITE_D1_MEM    0xb0
#define MAS_CMD_READ_REG        0xd0
#define MAS_CMD_READ_D0_MEM     0xe0
#define MAS_CMD_READ_D1_MEM     0xf0
#endif

int mas_default_read(unsigned short *buf);
int mas_run(unsigned short address);
int mas_readmem(int bank, int addr, unsigned long* dest, int len);
int mas_writemem(int bank, int addr, const unsigned long* src, int len);
int mas_readreg(int reg);
int mas_writereg(int reg, unsigned int val);
void mas_reset(void);
int mas_direct_config_read(unsigned char reg);
int mas_direct_config_write(unsigned char reg, unsigned int val);
int mas_codec_writereg(int reg, unsigned int val);
int mas_codec_readreg(int reg);
unsigned long mas_readver(void);

#endif

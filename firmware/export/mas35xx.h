/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Implementation of MAS35xx audiohw api driver.
 *
 * Copyright (C) 2007 by Christian Gmeiner
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

#ifndef _MAS35XX_H
#define _MAS35XX_H

#include "config.h"
#include "mascodec.h"

#define MAS_BANK_D0 0
#define MAS_BANK_D1 1

/* registers common to all MAS35xx */
#define MAS_REG_DCCF            0x8e
#define MAS_REG_MUTE            0xaa
#define MAS_REG_PIODATA         0xc8
#define MAS_REG_StartUpConfig   0xe6
#define MAS_REG_KPRESCALE       0xe7

#if CONFIG_CODEC == MAS3507D

/* I2C defines */
#define MAS_ADR         0x3a
#define MAS_DEV_WRITE   (MAS_ADR | 0x00)
#define MAS_DEV_READ    (MAS_ADR | 0x01)

/* MAS3507D registers */
#define MAS_DATA_WRITE   0x68
#define MAS_DATA_READ    0x69
#define MAS_CONTROL      0x6a

#define MAS_REG_KBASS           0x6b
#define MAS_REG_KTREBLE         0x6f

/* MAS3507D commands */
#define MAS_CMD_READ_ANCILLARY  0x30
#define MAS_CMD_WRITE_REG       0x90
#define MAS_CMD_WRITE_D0_MEM    0xa0
#define MAS_CMD_WRITE_D1_MEM    0xb0
#define MAS_CMD_READ_REG        0xd0
#define MAS_CMD_READ_D0_MEM     0xe0
#define MAS_CMD_READ_D1_MEM     0xf0

/* MAS3507D D0 memmory cells */
#define MAS_D0_MPEG_FRAME_COUNT  0x300
#define MAS_D0_MPEG_STATUS_1     0x301
#define MAS_D0_MPEG_STATUS_2     0x302
#define MAS_D0_CRC_ERROR_COUNT   0x303
#define MAS_D0_OUT_LL            0x7f8
#define MAS_D0_OUT_LR            0x7f9
#define MAS_D0_OUT_RL            0x7fa
#define MAS_D0_OUT_RR            0x7fb

#define VOLUME_MIN -780
#define VOLUME_MAX  180
#define AUDIOHW_CAPS (BASS_CAP | TREBLE_CAP | PRESCALER_CAP)

static const unsigned int bass_table[] =
{
    0x9e400, /* -15dB */
    0xa2800, /* -14dB */
    0xa7400, /* -13dB */
    0xac400, /* -12dB */
    0xb1800, /* -11dB */
    0xb7400, /* -10dB */
    0xbd400, /* -9dB */
    0xc3c00, /* -8dB */
    0xca400, /* -7dB */
    0xd1800, /* -6dB */
    0xd8c00, /* -5dB */
    0xe0400, /* -4dB */
    0xe8000, /* -3dB */
    0xefc00, /* -2dB */
    0xf7c00, /* -1dB */
    0,
    0x800,   /* 1dB */
    0x10000, /* 2dB */
    0x17c00, /* 3dB */
    0x1f800, /* 4dB */
    0x27000, /* 5dB */
    0x2e400, /* 6dB */
    0x35800, /* 7dB */
    0x3c000, /* 8dB */
    0x42800, /* 9dB */
    0x48800, /* 10dB */
    0x4e400, /* 11dB */
    0x53800, /* 12dB */
    0x58800, /* 13dB */
    0x5d400, /* 14dB */
    0x61800  /* 15dB */
};

static const unsigned int treble_table[] =
{
    0xb2c00, /* -15dB */
    0xbb400, /* -14dB */
    0xc1800, /* -13dB */
    0xc6c00, /* -12dB */
    0xcbc00, /* -11dB */
    0xd0400, /* -10dB */
    0xd5000, /* -9dB */
    0xd9800, /* -8dB */
    0xde000, /* -7dB */
    0xe2800, /* -6dB */
    0xe7e00, /* -5dB */
    0xec000, /* -4dB */
    0xf0c00, /* -3dB */
    0xf5c00, /* -2dB */
    0xfac00, /* -1dB */
    0,
    0x5400,  /* 1dB */
    0xac00,  /* 2dB */
    0x10400, /* 3dB */
    0x16000, /* 4dB */
    0x1c000, /* 5dB */
    0x22400, /* 6dB */
    0x28400, /* 7dB */
    0x2ec00, /* 8dB */
    0x35400, /* 9dB */
    0x3c000, /* 10dB */
    0x42c00, /* 11dB */
    0x49c00, /* 12dB */
    0x51800, /* 13dB */
    0x58400, /* 14dB */
    0x5f800  /* 15dB */
};

static const unsigned int prescale_table[] =
{
    0x80000,  /* 0db */
    0x8e000,  /* 1dB */
    0x9a400,  /* 2dB */
    0xa5800, /* 3dB */
    0xaf400, /* 4dB */
    0xb8000, /* 5dB */
    0xbfc00, /* 6dB */
    0xc6c00, /* 7dB */
    0xcd000, /* 8dB */
    0xd25c0, /* 9dB */
    0xd7800, /* 10dB */
    0xdc000, /* 11dB */
    0xdfc00, /* 12dB */
    0xe3400, /* 13dB */
    0xe6800, /* 14dB */
    0xe9400  /* 15dB */
};

#else /* CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F */

/* I2C defines */
#define MAS_ADR         0x3c
#define MAS_DEV_WRITE   (MAS_ADR | 0x00)
#define MAS_DEV_READ    (MAS_ADR | 0x01)

/* MAS3587F/MAS3539F registers */
#define MAS_DATA_WRITE   0x68
#define MAS_DATA_READ    0x69
#define MAS_CODEC_WRITE  0x6c
#define MAS_CODEC_READ   0x6d
#define MAS_CONTROL      0x6a
#define MAS_DCCF         0x76
#define MAS_DCFR         0x77

#define MAS_REG_KMDB_SWITCH     0x21
#define MAS_REG_KMDB_STR        0x22
#define MAS_REG_KMDB_HAR        0x23
#define MAS_REG_KMDB_FC         0x24
#define MAS_REG_KLOUDNESS       0x1e
#define MAS_REG_QPEAK_L         0x0a
#define MAS_REG_QPEAK_R         0x0b
#define MAS_REG_DQPEAK_L        0x0c
#define MAS_REG_DQPEAK_R        0x0d
#define MAS_REG_VOLUME_CONTROL  0x10
#define MAS_REG_BALANCE         0x11
#define MAS_REG_KAVC            0x12
#define MAS_REG_KBASS           0x14
#define MAS_REG_KTREBLE         0x15

/* MAS3587F/MAS3539F commands */
#define MAS_CMD_READ_ANCILLARY  0x50
#define MAS_CMD_FAST_PRG_DL     0x60
#define MAS_CMD_READ_IC_VER     0x70
#define MAS_CMD_READ_REG        0xa0
#define MAS_CMD_WRITE_REG       0xb0
#define MAS_CMD_READ_D0_MEM     0xc0
#define MAS_CMD_READ_D1_MEM     0xd0
#define MAS_CMD_WRITE_D0_MEM    0xe0
#define MAS_CMD_WRITE_D1_MEM    0xf0

/* MAS3587F D0 memory cells */
#if CONFIG_CODEC == MAS3587F
#define MAS_D0_APP_SELECT        0x7f6
#define MAS_D0_APP_RUNNING       0x7f7
#define MAS_D0_ENCODER_CONTROL   0x7f0
#define MAS_D0_IO_CONTROL_MAIN   0x7f1
#define MAS_D0_INTERFACE_CONTROL 0x7f2
#define MAS_D0_OFREQ_CONTROL     0x7f3
#define MAS_D0_OUT_CLK_CONFIG    0x7f4
#define MAS_D0_SPD_OUT_BITS      0x7f8
#define MAS_D0_SOFT_MUTE         0x7f9
#define MAS_D0_OUT_LL            0x7fc
#define MAS_D0_OUT_LR            0x7fd
#define MAS_D0_OUT_RL            0x7fe
#define MAS_D0_OUT_RR            0x7ff
#define MAS_D0_MPEG_FRAME_COUNT  0xfd0
#define MAS_D0_MPEG_STATUS_1     0xfd1
#define MAS_D0_MPEG_STATUS_2     0xfd2
#define MAS_D0_CRC_ERROR_COUNT   0xfd3

/* MAS3539F D0 memory cells */
#elif CONFIG_CODEC == MAS3539F
#define MAS_D0_APP_SELECT        0x34b
#define MAS_D0_APP_RUNNING       0x34c
/* no encoder :( */
#define MAS_D0_IO_CONTROL_MAIN   0x346
#define MAS_D0_INTERFACE_CONTROL 0x347
#define MAS_D0_OFREQ_CONTROL     0x348
#define MAS_D0_OUT_CLK_CONFIG    0x349
#define MAS_D0_SPD_OUT_BITS      0x351
#define MAS_D0_SOFT_MUTE         0x350
#define MAS_D0_OUT_LL            0x354
#define MAS_D0_OUT_LR            0x355
#define MAS_D0_OUT_RL            0x356
#define MAS_D0_OUT_RR            0x357
#define MAS_D0_MPEG_FRAME_COUNT  0xfd0
#define MAS_D0_MPEG_STATUS_1     0xfd1
#define MAS_D0_MPEG_STATUS_2     0xfd2
#define MAS_D0_CRC_ERROR_COUNT   0xfd3
#endif

/* MAS3587F and MAS3539F handle clipping prevention internally so we do not need
 * the prescaler -> CLIPPING_CAP
 */

#define VOLUME_MIN -400
#define VOLUME_MAX  600
#define AUDIOHW_CAPS (BASS_CAP | TREBLE_CAP | BALANCE_CAP | CLIPPING_CAP)

#endif /* CONFIG_CODEC */

/* Function prototypes */
#if CONFIG_CODEC == MAS3587F || CONFIG_CODEC == MAS3539F
extern void audiohw_set_loudness(int value);
extern void audiohw_set_avc(int value);
extern void audiohw_set_mdb_strength(int value);
extern void audiohw_set_mdb_harmonics(int value);
extern void audiohw_set_mdb_center(int value);
extern void audiohw_set_mdb_shape(int value);
extern void audiohw_set_mdb_enable(int value);
extern void audiohw_set_superbass(int value);
extern void audiohw_set_balance(int val);
extern void audiohw_set_pitch(unsigned long val);
#endif

#endif /* _MAS35XX_H */

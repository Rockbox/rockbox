/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Marcin Bukat
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

#define ID_READ                 0x000
#define DRIVER_OUT_CTRL         0x001
#define WAVEFORM_CTRL           0x002
#define ENTRY_MODE              0x003
/* 0x004 - 0x005 reserved */
#define SHAPENING_CTRL          0x006 /* not present in datasheet */
#define DISPLAY_CTRL1           0x007
#define DISPLAY_CTRL2           0x008
#define LOW_PWR_CTRL1           0x009
/* 0x00A reserved */
#define LOW_PWR_CTRL2           0x00B
#define EXT_DISP_CTRL1          0x00C
/* 0x00D - 0x00E reserved */
#define EXT_DISP_CTRL2          0x00F
#define PANEL_IF_CTRL1          0x010
#define PANEL_IF_CTRL2          0x011
#define PANEL_IF_CTRL3          0x012
/* 0x013 - 0x01F reserved */
#define PANEL_IF_CTRL4          0x020
#define PANEL_IF_CTRL5          0x021
#define PANEL_IF_CTRL6          0x022
/* 0x023 - 0x08F reserved */
#define FRAME_MKR_CTRL          0x090
/* 0x091 reserved */
#define MDDI_CTRL               0x092 /* not present in datasheet */
/* 0x093 - 0x0FF reserved */
#define PWR_CTRL1               0x100
#define PWR_CTRL2               0x101
#define PWR_CTRL3               0x102
#define PWR_CTRL4               0x103 /* amplitude to VCOM */
/* 0x104 - 0x106 reserved */
#define PWR_CTRL5               0x107
/* 0x108 - 0x10F reserved */
#define PWR_CTRL6               0x110
#define PWR_CTRL7               0x112 /* not present in datasheet */
/* 0x113 - 0x1FF reserved */
#define GRAM_H_ADDR             0x200
#define GRAM_V_ADDR             0x201
#define GRAM_READ               0x202
#define GRAM_WRITE              0x202
/* 0x203 - 0x20F reserved */
#define WINDOW_H_START          0x210
#define WINDOW_H_END            0x211
#define WINDOW_V_START          0x212
#define WINDOW_V_END            0x213
/* 0x214 - 0x27F reserved */
#define NVM_READ                0x280
#define NVM_WRITE               0x280
#define VCOM_HV1                0x281
#define VCOM_HV2                0x282
/* 0x283 - 0x2FF reserved */
#define GAMMA_CTRL1             0x300
#define GAMMA_CTRL2             0x301
#define GAMMA_CTRL3             0x302
#define GAMMA_CTRL4             0x303
#define GAMMA_CTRL5             0x304
#define GAMMA_CTRL6             0x305
#define GAMMA_CTRL7             0x306
#define GAMMA_CTRL8             0x307
#define GAMMA_CTRL9             0x308
#define GAMMA_CTRL10            0x309
#define GAMMA_CTRL11            0x30A
#define GAMMA_CTRL12            0x30B
#define GAMMA_CTRL13            0x30C
#define GAMMA_CTRL14            0x30D
#define GAMMA_CTRL15            0x30E
#define GAMMA_CTRL16            0x30F
/* 0x310 - 0x3FF reserved */
#define BASE_IMG_SIZE           0x400
#define BASE_IMG_CTRL           0x401
/* 0x402 - 0x403 reserved */
#define VSCROLL_CTRL            0x404
/* 0x405 - 0x4FF reserved */
#define PART1_POS               0x500
#define PART1_START             0x501
#define PART1_END               0x502
#define PART2_POS               0x503
#define PART2_START             0x504
#define PART2_END               0x505
/* 0x506 - 0x5FF reserved */
#define RESET                   0x600 /* not present in datasheet */
/* 0x601 - 0x605 */
#define IF_ENDIAN               0x606
/* 0x607 - 0x6EF reserved */
#define NVM_CTRL                0x6F0
/* 0x6F1 - 0xFFF reserved */

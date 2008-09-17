/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Frank Gevaerts
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
#ifndef _QT1106_H_
#define _QT1106_H_

#define QT1106_CT 0x00800000

#define QT1106_AKS_DISABLED 0x00000000
#define QT1106_AKS_GLOBAL   0x00010000
#define QT1106_AKS_MODE1    0x00020000
#define QT1106_AKS_MODE2    0x00030000
#define QT1106_AKS_MODE3    0x00040000
#define QT1106_AKS_MODE4    0x00050000

#define QT1106_SLD_WHEEL    0x00000000
#define QT1106_SLD_SLIDER   0x00080000

#define QT1106_KEY7_NORMAL  0x00000000
#define QT1106_KEY7_PROX    0x00100000

#define QT1106_MODE_FREE    0x00000000
#define QT1106_MODE_LP1     0x00000100
#define QT1106_MODE_LP2     0x00000200
#define QT1106_MODE_LP3     0x00000300
#define QT1106_MODE_LP4     0x00000400
#define QT1106_MODE_SYNC    0x00000500
#define QT1106_MODE_SLEEP   0x00000600

#define QT1106_LPB          0x00000800
#define QT1106_DI           0x00001000

#define QT1106_MOD_10       0x00000000
#define QT1106_MOD_20       0x00002000
#define QT1106_MOD_60       0x00004000
#define QT1106_MOD_INF      0x00006000

#define QT1106_CAL_ALL      0x00000000
#define QT1106_CAL_KEYS     0x00000008
#define QT1106_CAL_WHEEL    0x00000010
#define QT1106_RES_4        0x00000020
#define QT1106_RES_8        0x00000040
#define QT1106_RES_16       0x00000060
#define QT1106_RES_32       0x00000080
#define QT1106_RES_64       0x000000A0
#define QT1106_RES_128      0x000000C0
#define QT1106_RES_256      0x000000E0


void init_qt1106(void);
void qt1106_wait(void);
unsigned int qt1106_io(unsigned int input);

#endif


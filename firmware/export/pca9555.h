/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 Andrew Ryabinin
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

#ifndef _PCA9555_H
#define _PCA9555_H

#include "pca9555-target.h"

#define PCA9555_IN0_CMD      0
#define PCA9555_IN1_CMD      1
#define PCA9555_OUT0_CMD     2
#define PCA9555_OUT1_CMD     3
#define PCA9555_POL0_INV_CMD 4
#define PCA9555_POL1_INV_CMD 5
#define PCA9555_CFG0_CMD     6
#define PCA9555_CFG1_CMD     7

#define PCA9555_IN_CMD       PCA9555_IN0_CMD
#define PCA9555_OUT_CMD      PCA9555_OUT0_CMD
#define PCA9555_POL_INV_CMD  PCA9555_POL0_INV_CMD
#define PCA9555_CFG_CMD      PCA9555_CFG0_CMD

void pca9555_target_init(void);
void pca9555_init(void);
unsigned short pca9555_read_input(void);
unsigned short pca9555_read_output(void);
unsigned short pca9555_read_config(void);
void pca9555_write_output(const unsigned short data, const unsigned short mask);
void pca9555_write_config(const unsigned short data, const unsigned short mask);

#endif

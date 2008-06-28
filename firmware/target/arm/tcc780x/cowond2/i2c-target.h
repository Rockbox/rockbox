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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef I2C_TARGET_H
#define I2C_TARGET_H

/* Definitions for the D2 I2C bus */

#define SCL_BIT (1<<0)
#define SDA_BIT (1<<1)

#define SCL    (GPIOA & SCL_BIT)
#define SCL_HI GPIOA_SET = SCL_BIT
#define SCL_LO GPIOA_CLEAR = SCL_BIT

#define SDA        (GPIOA & SDA_BIT)
#define SDA_HI     GPIOA_SET = SDA_BIT
#define SDA_LO     GPIOA_CLEAR = SDA_BIT
#define SDA_INPUT  GPIOA_DIR &= ~SDA_BIT
#define SDA_OUTPUT GPIOA_DIR |= SDA_BIT

#endif /* I2C_TARGET_H */

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
#ifndef I2C_TARGET_H
#define I2C_TARGET_H

/* Definitions for the TCC77X I2C bus */

#define SDA_BIT (1<<10)
#define SCL_BIT (1<<11)

#define SCL    (GPIOB & SCL_BIT)
#define SCL_HI GPIOB |= SCL_BIT
#define SCL_LO GPIOB &= ~SCL_BIT

#define SDA        (GPIOB & SDA_BIT)
#define SDA_HI     GPIOB |= SDA_BIT
#define SDA_LO     GPIOB &= ~SDA_BIT
#define SDA_INPUT  GPIOB_DIR &= ~SDA_BIT
#define SDA_OUTPUT GPIOB_DIR |= SDA_BIT

#endif /* I2C_TARGET_H */

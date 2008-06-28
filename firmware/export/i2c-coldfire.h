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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 *  Driver for MCF52xx's I2C interface
 *  2005-02-17 hubble@mochine.com
 *
 */
 
#ifndef _I2C_COLDFIRE_H
#define _I2C_COLDFIRE_H

#include "cpu.h"

void i2c_init(void);
int i2c_read (volatile unsigned char *iface, unsigned char addr, 
              unsigned char *buf, int count);
int i2c_write(volatile unsigned char *iface, unsigned char addr, 
              const unsigned char *buf, int count);
void i2c_close(void);
void i2c_adjust_prescale(int multiplier);

#define I2C_IFACE_0  ((volatile unsigned char *)&MADR)
#define I2C_IFACE_1  ((volatile unsigned char *)&MADR2)

#define MAX_LOOP    0x100     /* TODO: select a better value */

/* PLLCR control */
#define QSPISEL (1 << 11)		/* Selects QSPI or I2C interface */

/* Offsets to I2C registers from base address */
#define O_MADR	0x00            /* Slave Address        */
#define O_MFDR	0x04            /* Frequency divider    */
#define O_MBCR	0x08            /* Control register     */
#define O_MBSR	0x0c            /* Status register      */
#define O_MBDR	0x10            /* Data register        */

/* MBSR - Status register */
#define ICF     (1 << 7)        /* Transfer Complete	*/
#define IAAS    (1 << 6)        /* Addressed As Alave	*/
#define IBB     (1 << 5)        /* Bus Busy		        */
#define IAL     (1 << 4)        /* Arbitration Lost	    */
#define SRW     (1 << 2)        /* Slave R/W		    */
#define IIF     (1 << 1)        /* I2C Interrupt	    */
#define RXAK    (1 << 0)        /* No Ack bit		    */

/* MBCR - Control register */
#define IEN     (1 << 7)		/* I2C Enable		    */
#define IIEN    (1 << 6)		/* Interrupt Enable	    */
#define MSTA    (1 << 5)		/* Master/Slave select	*/
#define MTX     (1 << 4)		/* Transmit/Receive	    */
#define TXAK    (1 << 3)		/* Transfer ACK		    */
#define RSTA    (1 << 2)		/* Restart..		    */


#endif

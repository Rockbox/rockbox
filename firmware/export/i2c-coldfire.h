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

/*
 *  Driver for MCF52xx's I2C interface
 *  2005-02-17 hubble@mochine.com
 *
 */
 
#ifndef _I2C_COLDFIRE_H
#define _I2C_COLDFIRE_H

void i2c_init(void);
int i2c_write(int device, unsigned char *buf, int count);
void i2c_close(void);


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
#define IFF     (1 << 1)        /* I2C Interrupt	    */
#define RXAK    (1 << 0)        /* No Ack bit		    */

/* MBCR - Control register */
#define IEN     (1 << 7)		/* I2C Enable		    */
#define IIEN    (1 << 6)		/* Interrupt Enable	    */
#define MSTA    (1 << 5)		/* Master/Slave select	*/
#define MTX     (1 << 4)		/* Transmit/Receive	    */
#define TXAK    (1 << 3)		/* Transfer ACK		    */
#define RSTA    (1 << 2)		/* Restart..		    */


#endif

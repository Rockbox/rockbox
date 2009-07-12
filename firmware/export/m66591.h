/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Karl Kurbjun
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
 
#include "config.h"

#define M66591_REG(addr)  (*(volatile unsigned short *) \
    ((unsigned char *) M66591_BASE + (addr)) )

/* Interrupt handler routine, visible for target handler */
void USB_DEVICE(void);

/* Register offsets */
#define M66591_TRN_CTRL         M66591_REG(0x00) /* pg 14 */
#define M66591_TRN_LNSTAT       M66591_REG(0x02) /* pg 16 */

#define M66591_HSFS             M66591_REG(0x04) /* pg 17 */
#define M66591_TESTMODE         M66591_REG(0x06) /* pg 18 */

#define M66591_PIN_CFG0         M66591_REG(0x08) /* pg 19 */
#define M66591_PIN_CFG1         M66591_REG(0x0A) /* pg 20 */
#define M66591_PIN_CFG2         M66591_REG(0x0C) /* pg 21 */

#define M66591_CPORT            M66591_REG(0x14) /* pg 23 */
#define M66591_DPORT            M66591_REG(0x18) /* pg 24 */

#define M66591_DCP_CTRLEN       M66591_REG(0x26) /* pg 25 */

#define M66591_CPORT_CTRL0      M66591_REG(0x28) /* pg 26 */
#define M66591_CPORT_CTRL1      M66591_REG(0x2C) /* pg 28 */
#define M66591_CPORT_CTRL2      M66591_REG(0x2E) /* pg 30 */

#define M66591_DPORT_CTRL0      M66591_REG(0x30) /* pg 31 */
#define M66591_DPORT_CTRL1      M66591_REG(0x34) /* pg 34 */
#define M66591_DPORT_CTRL2      M66591_REG(0x36) /* pg 36 */

#define M66591_INTCFG_MAIN      M66591_REG(0x40) /* pg 37 */
#define M66591_INTCFG_OUT       M66591_REG(0x42) /* pg 40 */
#define M66591_INTCFG_RDY       M66591_REG(0x44) /* pg 41 */
#define M66591_INTCFG_NRDY      M66591_REG(0x48) /* pg 42 */
#define M66591_INTCFG_EMP       M66591_REG(0x4C) /* pg 43 */

#define M66591_INTSTAT_MAIN     M66591_REG(0x60) /* pg 44 */
#define M66591_INTSTAT_RDY      M66591_REG(0x64) /* pg 48 */
#define M66591_INTSTAT_NRDY     M66591_REG(0x68) /* pg 50 */
#define M66591_INTSTAT_EMP      M66591_REG(0x6C) /* pg 53 */

#define M66591_USB_ADDRESS      M66591_REG(0x74) /* pg 56 */

#define M66591_USB_REQ0         M66591_REG(0x78) /* pg 57 */
#define M66591_USB_REQ1         M66591_REG(0x7A) /* pg 58 */
#define M66591_USB_REQ2         M66591_REG(0x7C) /* pg 59 */
#define M66591_USB_REQ3         M66591_REG(0x7E) /* pg 60 */

#define M66591_DCP_CNTMD        M66591_REG(0x82) /* pg 61 */
#define M66591_DCP_MXPKSZ       M66591_REG(0x84) /* pg 62 */
#define M66591_DCPCTRL          M66591_REG(0x88) /* pg 63 */

#define M66591_PIPE_CFGSEL      M66591_REG(0x8C) /* pg 65 */
#define M66591_PIPE_CFGWND      M66591_REG(0x90) /* pg 66 */

#define M66591_PIPECTRL1        M66591_REG(0xA0) /* pg 69 */
#define M66591_PIPECTRL2        M66591_REG(0xA2) /* pg 69 */
#define M66591_PIPECTRL3        M66591_REG(0xA4) /* pg 69 */
#define M66591_PIPECTRL4        M66591_REG(0xA6) /* pg 69 */
#define M66591_PIPECTRL5        M66591_REG(0xA8) /* pg 71 */
#define M66591_PIPECTRL6        M66591_REG(0xAA) /* pg 71 */

/* These defines are used for CTRL register handshake setup 
 *  They are used on the following registers:
 *  DCPCTRL and PIPECTRL(1-6)
 */
#define PIPE_SHAKE_NAK          0x00
#define PIPE_SHAKE_BUF          0x01
#define PIPE_SHAKE_STALL        0x02

/* These defines are used for the control transfer stage status */
#define CTRL_IDLE               0x00 /* Idle Stage */
#define CTRL_RTDS               0x01 /* Read transfer data stage */
#define CTRL_RTSS               0x02 /* Read transfer status stage */
#define CTRL_WTDS               0x03 /* Write transfer data stage */
#define CTRL_WTSS               0x04 /* Write transfer status stage */
#define CTRL_WTND               0x05 /* Write transfer no data stage */
#define CTRL_TRER               0x06 /* Transmit error stage */


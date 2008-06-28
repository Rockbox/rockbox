/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Catalin Patulea
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

#ifndef REGISTERS_H
#define REGISTERS_H

#define C5409_REG(addr)                 (*(volatile unsigned short *)(addr))

/* This is NOT good for reading, only for writing. */
#define BANKED_REG(spsa, spsd, subaddr) spsa = (subaddr), spsd

#define IMR    C5409_REG(0x00)
#define IFR    C5409_REG(0x01)

#define PMST   C5409_REG(0x1D)

#define TCR    C5409_REG(0x26)

/* McBSP 0 (SPRU302 Chapter 2) */
#define DXR20  C5409_REG(0x22)
#define DXR10  C5409_REG(0x23)
#define SPSA0  C5409_REG(0x38)
#define SPSD0  C5409_REG(0x39)
#define SPCR10 BANKED_REG(SPSA0, SPSD0, 0x00)
#define SPCR20 BANKED_REG(SPSA0, SPSD0, 0x01)
#define RCR10  BANKED_REG(SPSA0, SPSD0, 0x02)
#define RCR20  BANKED_REG(SPSA0, SPSD0, 0x03)
#define XCR10  BANKED_REG(SPSA0, SPSD0, 0x04)
#define XCR20  BANKED_REG(SPSA0, SPSD0, 0x05)
#define SRGR10 BANKED_REG(SPSA0, SPSD0, 0x06)
#define SRGR20 BANKED_REG(SPSA0, SPSD0, 0x07)
#define PCR0   BANKED_REG(SPSA0, SPSD0, 0x0e)

/* McBSP 1 */
#define DXR21  C5409_REG(0x42)
#define DXR11  C5409_REG(0x43)
#define SPSA1  C5409_REG(0x48)
#define SPSD1  C5409_REG(0x49)
#define SPCR11 BANKED_REG(SPSA1, SPSD1, 0x00)
#define SPCR21 BANKED_REG(SPSA1, SPSD1, 0x01)
#define XCR11  BANKED_REG(SPSA1, SPSD1, 0x04)
#define XCR21  BANKED_REG(SPSA1, SPSD1, 0x05)
#define PCR1   BANKED_REG(SPSA1, SPSD1, 0x0e)

/* DMA */
#define DMPREC C5409_REG(0x54)
#define DMSA   C5409_REG(0x55)
#define DMSDI  C5409_REG(0x56)
#define DMSDN  C5409_REG(0x57)
#define DMSRC0 BANKED_REG(DMSA, DMSDN, 0x00)
#define DMDST0 BANKED_REG(DMSA, DMSDN, 0x01)
#define DMCTR0 BANKED_REG(DMSA, DMSDN, 0x02)
#define DMSFC0 BANKED_REG(DMSA, DMSDN, 0x03)
#define DMMCR0 BANKED_REG(DMSA, DMSDN, 0x04)


/* DM320 */
ioport unsigned short port280;
#define CP_INTC       port280
ioport unsigned short port8000;
#define SDEM_ADDRL    port8000
ioport unsigned short port8001;
#define SDEM_ADDRH    port8001
ioport unsigned short port8002;
#define DSP_ADDRL     port8002
ioport unsigned short port8003;
#define DSP_ADDRH     port8003
ioport unsigned short port8004;
#define DMA_SIZE      port8004
ioport unsigned short port8005;
#define DMA_CTRL      port8005
ioport unsigned short port8006;
#define DMA_TRG       port8006
ioport unsigned short port8007;
#define DMA_REST      port8007

#endif

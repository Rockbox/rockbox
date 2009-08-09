/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Bertrik Sikken
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
#include <string.h>

#include "config.h"
#include "s5l8700.h"
#include "nand-target.h"

/*  Driver for the S5L8700 flash memory controller for low-level access to the
    NAND flash of the Samsung YP-S3.

    The YP-S3 seems to use the pins P6.5 and P6.6 as chip selects in GPIO mode
    instead of using the regular pins P6.3 and P6.4.
*/

#define FMSTAT_RBB              (1<<0)
#define FMSTAT_RBBDone          (1<<1)
#define FMSTAT_CMDDone          (1<<2)
#define FMSTAT_AddrDone         (1<<3)
#define FMSTAT_TransDone        (1<<4)
#define FMSTAT_WFIFO_HEMPTY     (1<<5)
#define FMSTAT_RFIFO_HFULL      (1<<6)
#define FMSTAT_WFIFO_EMPTY      (1<<8)
#define FMSTAT_RFIFO_FULL       (1<<9)
#define FMSTAT_EndECC           (1<<10)

#define FMCTRL1_DoTransAddr     (1<<0)
#define FMCTRL1_DoReadData      (1<<1)
#define FMCTRL1_DoWriteData     (1<<2)
#define FMCTRL1_WriteREQSEL     (1<<4)
#define FMCTRL1_ClearSyndPtr    (1<<5)
#define FMCTRL1_ClearWFIFO      (1<<6)
#define FMCTRL1_ClearRFIFO      (1<<7)
#define FMCTRL1_ParityPtr       (1<<8)
#define FMCTRL1_SyndPtr         (1<<9)

static void nand_chip_select(int bank)
{
    unsigned int select;

    select = (1 << bank);
    FMCTRL0 = 0x1821 | ((select & 3) << 1);
    PDAT6 = (PDAT6 & ~0x60) | ((~select & 0xC) << 3);
}

void nand_ll_init(void)
{
    /* enable flash memory controller */
    PWRCON &= ~(1 << 1);

    /* P2.X is SMC I/O */
    PCON2 = 0x55555555;
    /* P4.1 = CLE, P4.4 = nWR, P4.5 = nRD */
    PCON4 = (PCON4 & ~0x00FF00F0) | 0x00550050;
    /* P6.0 = nf_rbn, P6.1 = smc_ce0, P6.2 = smc_ce1,
       P6.5 = smc_ce2 (as GPIO), P6.6 = smc_ce3 (as GPIO), P6.7 = ALE */
    PCON6 = (PCON6 & ~0xFFF00FFF) | 0x51100555;
    PDAT6 |= 0x60;
}

unsigned int nand_ll_read_id(int bank)
{
    unsigned int nand_id;

    nand_chip_select(bank);
    
    /* send "read id" command */
    FMCMD = 0x90;
    while ((FMCSTAT & FMSTAT_CMDDone) == 0);
    FMCSTAT = FMSTAT_CMDDone;
    
    /* transfer address */
    FMANUM = 0;
    FMADDR0 = 0;
    FMCTRL1 = FMCTRL1_DoTransAddr;
    while ((FMCSTAT & FMSTAT_AddrDone) == 0);
    FMCSTAT = FMSTAT_AddrDone;
    
    /* read back data */
    FMDNUM = 3;
    FMCTRL1 = FMCTRL1_DoReadData;
    while ((FMCSTAT & FMSTAT_TransDone) == 0);
    FMCSTAT = FMSTAT_TransDone;
    nand_id = FMFIFO;
    
    /* clear read FIFO */
    FMCTRL1 = FMCTRL1_ClearRFIFO;
    
    return nand_id;    
}


/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2007 by Jonathan Gordon
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
#ifndef __TSC2100_H_
#define __TSC2100_H_

/* Read X, Y, Z1, Z2 touchscreen coordinates. */
void tsc2100_read_values(short *x, short* y, short *z1, short *z2);

/* read a register */
short tsc2100_readreg(int page, int address);
/* write a value to the register */
void tsc2100_writereg(int page, int address, short value);

/* ts adc page defines (page 1, 00h ) (refer to page 40 of the datasheet) */
#define TSADC_PAGE          1
#define TSADC_ADDRESS       0x00
#define TSADC_PSTCM         (1<<15)
#define TSADC_ADST          (1<<14)
#define TSADC_ADSCM_MASK    (0x3C00)
#define TSADC_ADSCM_SHIFT   10
#define TSADC_RESOL_MASK    (0x0300)
#define TSADC_RESOL_SHIFT   8
#define TSADC_ADAVG_MASK    (0x00C0)
#define TSADC_ADAVG_SHIFT   6
#define TSADC_ADCR_MASK     (0x0030)
#define TSADC_ADCR_SHIFT    4
#define TSADC_PVSTC_MASK    (0x000E)
#define TSADC_PVSTC_SHIFT   1
#define TSADC_AVGFS         (1<<0)

/* ts status page defines (page 1, 01h ) (refer to page 41 of the datasheet) */
#define TSSTAT_PAGE             1
#define TSSTAT_ADDRESS          0x01
#define TSSTAT_PINTDAV_MASK     0xC000 /* controls the !PINTDAV pin */
#define TSSTAT_PINTDAV_SHIFT    14
/* these are all read only */
#define TSSTAT_PWRDN            (1<<13)
#define TSSTAT_HCTLM            (1<<12)
#define TSSTAT_DAVAIL           (1<<11)
#define TSSTAT_XSTAT            (1<<10)
#define TSSTAT_YSTAT            (1<<9)
#define TSSTAT_Z1STAT           (1<<8)
#define TSSTAT_Z2STAT           (1<<7)
#define TSSTAT_B1STAT           (1<<6)
#define TSSTAT_B2STAT           (1<<5)
#define TSSTAT_AXSTAT           (1<<4)
// Bit 3 is reserved            (1<<3)
#define TSSTAT_T1STAT           (1<<2)
#define TSSTAT_T2STAT           (1<<1)
// Bit 0 is reserved            (1<<0)

/* ts Reset Control */
#define TSRESET_PAGE    1
#define TSRESET_ADDRESS 0x04
#define TSRESET_VALUE   0xBB00

/* ts codec dac gain control */
#define TSDACGAIN_PAGE          2
#define TSDACGAIN_ADDRESS       0x02
#define VOLUME_MAX  0
#define VOLUME_MIN  -630

/* ts audio control 1*/
#define TSAC1_PAGE          2
#define TSAC1_ADDRESS       0x00

/* ts audio control 2 */
#define TSAC2_PAGE          2
#define TSAC2_ADDRESS       0x04
#define TSAC2_KCLEN         (1<<15)
#define TSAC2_KCLAC_MASK    0x7000
#define TSAC2_KCLSC_SHIFT   12
#define TSAC2_APGASS        (1<<11)
#define TSAC2_KCLFRQ_MASK   0x0700
#define TSAC2_KCLFRQ_SHIFT  8
#define TSAC2_KCLLN_MASK    0x00F0
#define TSAC2_KCLLN_SHIFT   4
#define TSAC2_DLGAF         (1<<3) /* r only */
#define TSAC2_DRGAF         (1<<2) /* r only */
#define TSAC2_DASTC         (1<<1)
#define TSAC2_ADGAF         (1<<0) /* r only */

/* ts codec power control */
#define TSCPC_PAGE          2
#define TSCPC_ADDRESS       0x05

/* ts audio control 3 */
#define TSAC3_PAGE          2
#define TSAC3_ADDRESS       0x06

/* ts audio control 4 */
#define TSAC4_PAGE          2
#define TSAC4_ADDRESS       0x1d
#define TSAC4_ASTDP         (1<<15)
#define TSAC4_DASTDP        (1<<14)
#define TSAC4_ASSTDP        (1<<13)
#define TSAC4_DSTDP         (1<<12)
#define TSAC4_RESERVEDD11   (1<<11)
#define TSAC4_AGC_HYST_MASK 0x0c00
#define TSAC4_AGC_HYST_SHIFT 10
#define TSAC4_SHCKT_DIS     (1<<8)
#define TSAC4_SHCKT_PD      (1<<7)
#define TSAC4_SHCKT_FLAG    (1<<6)
#define TSAC4_DAC_POP_RED   (1<<5)
#define TSAC4_DAC_POP_RED_SET1          (1<<4)
#define TSAC4_DAC_POP_RED_SET2_MASK     0x000c
#define TSAC4_DAC_POP_RED_SET2_SHIFT    3
#define TSAC4_PGID_MASK     0x0003
#define TSAC4_PGID_SHIFT    0

/* ts audio control 5 */
#define TSAC5_PAGE          2
#define TSAC5_ADDRESS       0x1e

extern int tenthdb2master(int db);
extern void audiohw_init(void);
extern void audiohw_set_master_vol(int vol_l, int vol_r);
extern void audiohw_set_lineout_vol(int vol_l, int vol_r);
extern void audiohw_mute(bool mute);

#endif

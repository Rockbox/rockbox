/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Michael Sevakis
 *
 * i.MX31 DVFS and DPTC driver declarations
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

#ifndef _DVFS_DPTC_IMX31_H_
#define _DVFS_DPTC_IMX31_H_

/* DVFS load tracking signals */
enum DVFS_LT_SIGS
{
    DVFS_LT_SIG_M3IF_M0_BUF    = 0,  /* Hready signal of M3IF's master #0
                                        (L2 Cache) */
    DVFS_LT_SIG_M3IF_M1        = 1,  /* Hready signal of M3IF's master #1
                                        (L2 Cache) */
    DVFS_LT_SIG_MBX_MBXCLKGATE = 2,  /* Hready signal of M3IF's master #2
                                        (MBX) */
    DVFS_LT_SIG_M3IF_M3        = 3,  /* Hready signal of M3IF's master #3
                                        (MAX) */
    DVFS_LT_SIG_M3IF_M4        = 4,  /* Hready signal of M3IF's master #4
                                        (SDMA) */
    DVFS_LT_SIG_M3IF_M5        = 5,  /* Hready signal of M3IF's master #5
                                        (mpeg4_vga_encoder) */
    DVFS_LT_SIG_M3IF_M6        = 6,  /* Hready signal of M3IF's master #6
                                        (IPU) */
    DVFS_LT_SIG_M3IF_M7        = 7,  /* Hready signal of M3IF's master #7
                                        (IPU) */
    DVFS_LT_SIG_ARM11_P_IRQ_B_RBT_GATE = 8, /* ARM normal interrupt */
    DVFS_LT_SIG_ARM11_P_FIQ_B_RBT_GATE = 9, /* ARM fast interrupt */
    DVFS_LT_SIG_IPI_GPIO1_INT0 = 10, /* Interrupt line from GPIO */
    DVFS_LT_SIG_IPI_INT_IPU_FUNC = 11, /* Interrupt line from IPU */
    DVFS_LT_SIG_DVGP0          = 12, /* Software-controllable general-purpose
                                        bits from the CCM */
    DVFS_LT_SIG_DVGP1          = 13, /* Software-controllable general-purpose
                                        bits from the CCM */
    DVFS_LT_SIG_DVGP2          = 14, /* Software-controllable general-purpose
                                        bits from the CCM */
    DVFS_LT_SIG_DVGP3          = 15, /* Software-controllable general-purpose
                                        bits from the CCM */
};


enum DVFS_DVGPS
{
    DVFS_DVGP_0 = 0,
    DVFS_DVGP_1,
    DVFS_DVGP_2,
    DVFS_DVGP_3,
};

union dvfs_dptc_voltage_table_entry
{
    uint8_t sw[4];        /* Access as array            */

    struct
    {
                          /* Chosen by PMIC pin states  */
                          /* when SWxABDVS bit is 1:    */
                          /* DVSSWxA  DVSSWxB           */
        uint8_t sw1a;     /*    0        0              */
        uint8_t sw1advs;  /*    1        0              */
        uint8_t sw1bdvs;  /*    0        1              */
        uint8_t sw1bstby; /*    1        1              */
    };
};


struct dptc_dcvr_table_entry
{
    uint32_t dcvr0; /* DCVR register values for working point */
    uint32_t dcvr1;
    uint32_t dcvr2;
    uint32_t dcvr3;
};


struct dvfs_clock_table_entry
{
    uint32_t pll_val;     /* Setting for target PLL */
    uint32_t pdr_val;     /* Post-divider for target setting */
    uint32_t pll_num : 1; /* 1 = MCU PLL, 0 = Serial PLL */
    uint32_t vscnt   : 3; /* Voltage scaling counter, CKIL delay */
};


struct dvfs_lt_signal_descriptor
{
    uint8_t weight   : 3; /* Signal weight = 0-7 */
    uint8_t detect   : 1; /* 1 = edge-detected */
};

#define DVFS_NUM_LEVELS 4
#define DPTC_NUM_WP    17

/* 0 and 3 are *required*. DVFS hardware depends upon DVSUP pins showing
 * minimum (11) and maximum (00) levels or interrupts will be continuously
 * asserted. */
#define DVFS_LEVEL_0   (1u << 0)
#define DVFS_LEVEL_1   (1u << 1)
#define DVFS_LEVEL_2   (1u << 2)
#define DVFS_LEVEL_3   (1u << 3)

extern long cpu_voltage_setting;

void dvfs_dptc_init(void);
void dvfs_dptc_start(void);
void dvfs_dptc_stop(void);

void dvfs_wfi_monitor(bool on);
void dvfs_set_lt_weight(enum DVFS_LT_SIGS index, unsigned long value);
void dvfs_set_lt_detect(enum DVFS_LT_SIGS index, bool edge);
void dvfs_set_gp_bit(enum DVFS_DVGPS dvgp, bool assert);

unsigned int dvfs_dptc_get_voltage(void);
unsigned int dvfs_get_level(void);
void dvfs_set_level(unsigned int level);

unsigned int dptc_get_wp(void);
void dptc_set_wp(unsigned int wp);

#endif /* _DVFS_DPTC_IMX31_H_ */

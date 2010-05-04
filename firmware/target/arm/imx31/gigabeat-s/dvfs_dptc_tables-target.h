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
 * Target-specific i.MX31 DVFS and DPTC driver declarations
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
#ifndef _DVFS_DPTC_TARGET_H_
#define _DVFS_DPTC_TARGET_H_

#define DVFS_LEVEL_DEFAULT   1 /* 264 MHz - safe frequency for 1.35V */
#define DVFS_NUM_LEVELS      3 /* 528 MHz, 264 MHz, 132 MHz */
#define DVFS_NO_PWRRDY         /* PWRRDY is connected to different SoC port */

#define DPTC_WP_DEFAULT      1 /* 1.600, 1.350, 1.350 */
#define DPTC_WP_PANIC        3 /* Up to minimum for > 400 MHz */
#define DPTC_NUM_WP         17

#define VOLTAGE_SETTING_MIN     MC13783_SW_1_350
#define VOLTAGE_SETTING_MAX     MC13783_SW_1_625

/* Frequency increase threshold. Increase frequency change request
 * will be sent if DVFS counter value will be more than this value. */
#define DVFS_UPTHR      30

/* Frequency decrease threshold. Decrease frequency change request
 * will be sent if DVFS counter value will be less than this value. */
#define DVFS_DNTHR      18

/* Panic threshold. Panic frequency change request
 * will be sent if DVFS counter value will be more than this value. */
#define DVFS_PNCTHR     63

/* With the ARM clocked at 532, this setting yields a DIV_3_CLK of 2.03 kHz.
 *
 * Note: To get said clock, the divider would have to be 262144. The values
 * and their meanings are not published in the reference manual for i.MX31
 * but show up in the i.MX35 reference manual. Either that chip is different
 * and the values have an additional division or the comments in the BSP are
 * incorrect.
 */
#define DVFS_DIV3CK     CCM_LTR0_DIV3CK_131072

/* UPCNT defines the amount of times the up threshold should be exceeded
 * before DVFS will trigger frequency increase request. */

#if 0
/* Freescale BSP value: a bit too agressive IMHO */
#define DVFS_UPCNT       0x33
#endif
#define DVFS_UPCNT       0x48

/* DNCNT defines the amount of times the down threshold should be undershot
 * before DVFS will trigger frequency decrease request. */
#define DVFS_DNCNT       0x33

/* EMAC defines how many samples are included in EMA calculation */
#define DVFS_EMAC        0x20

/* Define mask of which reference circuits are employed for DPTC */
#define DPTC_DRCE_MASK (CCM_PMCR0_DRCE1 | CCM_PMCR0_DRCE3)

/* Due to a hardware bug in chip revisions < 2.0, when switching between
 * Serial and MCU PLLs, DVFS forces the target PLL to go into reset and
 * relock, only post divider frequency scaling is possible.
 */

static const union dvfs_dptc_voltage_table_entry
dvfs_dptc_voltage_table[DPTC_NUM_WP] =
{
    /* For each working point, there are four DVFS settings, chosen by the
     * DVS pin states on the PMIC set by the DVFS routines. Pins are reversed
     * and actual order as used by PMIC for DVSUP values of 00, 01, 10 and 11
     * is below.
     *
     *         SW1A            SW1ADVS           SW1BDVS           SW1BSTBY
     *  0                 2                 1                 3                */
    { { MC13783_SW_1_625, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_600, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_575, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_550, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_525, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_500, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_475, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_450, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_425, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_400, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_375, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_325, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_300, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_275, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_250, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
    { { MC13783_SW_1_225, MC13783_SW_1_350, MC13783_SW_1_350, MC13783_SW_1_350 } },
};

#if 1
/* For 27 MHz PLL reference clock */
static const struct dptc_dcvr_table_entry
dptc_dcvr_table[DVFS_NUM_LEVELS][DPTC_NUM_WP] =
{
        /*  DCVR0       DCVR1       DCVR2       DCVR3    */
    {   /* 528 MHz */
        { 0xffc00000, 0x90400000, 0xffc00000, 0xdd000000 },
        { 0xffc00000, 0x90629890, 0xffc00000, 0xdd34ed20 },
        { 0xffc00000, 0x90629890, 0xffc00000, 0xdd34ed20 },
        { 0xffc00000, 0x90629894, 0xffc00000, 0xdd74fd24 },
        { 0xffc00000, 0x90a2a894, 0xffc00000, 0xddb50d28 },
        { 0xffc00000, 0x90e2b89c, 0xffc00000, 0xde352d30 },
        { 0xffc00000, 0x9162d8a0, 0xffc00000, 0xdef55d38 },
        { 0xffc00000, 0x91e2f8a8, 0xffc00000, 0xdfb58d44 },
        { 0xffc00000, 0x926308b0, 0xffc00000, 0xe0b5cd54 },
        { 0xffc00000, 0x92e328bc, 0xffc00000, 0xe1f60d64 },
        { 0xffc00000, 0x93a358c0, 0xffc00000, 0xe3365d74 },
        { 0xffc00000, 0xf66388cc, 0xffc00000, 0xf6768d84 },
        { 0xffc00000, 0xf663b8d4, 0xffc00000, 0xf676dd98 },
        { 0xffc00000, 0xf663e8e0, 0xffc00000, 0xf6773da4 },
        { 0xffc00000, 0xf66418ec, 0xffc00000, 0xf6778dbc },
        { 0xffc00000, 0xf66458fc, 0xffc00000, 0xf677edd0 },
        { 0xffc00000, 0xf6648908, 0xffc00000, 0xf6783de8 },

    },
    {   /* 264 MHz */
        { 0xffc00000, 0x90400000, 0xffc00000, 0xdd000000 },
        { 0xffc00000, 0x9048a224, 0xffc00000, 0xdd0d4348 },
        { 0xffc00000, 0x9048a224, 0xffc00000, 0xdd0d4348 },
        { 0xffc00000, 0x9048a224, 0xffc00000, 0xdd4d4348 },
        { 0xffc00000, 0x9088b228, 0xffc00000, 0xdd8d434c },
        { 0xffc00000, 0x90c8b228, 0xffc00000, 0xde0d534c },
        { 0xffc00000, 0x9148b228, 0xffc00000, 0xdecd5350 },
        { 0xffc00000, 0x91c8c22c, 0xffc00000, 0xdf8d6354 },
        { 0xffc00000, 0x9248d22c, 0xffc00000, 0xe08d7354 },
        { 0xffc00000, 0x92c8d230, 0xffc00000, 0xe1cd8358 },
        { 0xffc00000, 0x9388e234, 0xffc00000, 0xe30d935c },
        { 0xffc00000, 0xf648e234, 0xffc00000, 0xf64db364 },
        { 0xffc00000, 0xf648f238, 0xffc00000, 0xf64dc368 },
        { 0xffc00000, 0xf648f23c, 0xffc00000, 0xf64dd36c },
        { 0xffc00000, 0xf649023c, 0xffc00000, 0xf64de370 },
        { 0xffc00000, 0xf649123c, 0xffc00000, 0xf64df374 },
        { 0xffc00000, 0xf6492240, 0xffc00000, 0xf64e1378 },

    },
    {   /* 132 MHz */
        { 0xffc00000, 0x90400000, 0xffc00000, 0xdd000000 },
        { 0xffc00000, 0x9048a224, 0xffc00000, 0xdd0d4348 },
        { 0xffc00000, 0x9048a224, 0xffc00000, 0xdd0d4348 },
        { 0xffc00000, 0x9048a224, 0xffc00000, 0xdd4d4348 },
        { 0xffc00000, 0x9088b228, 0xffc00000, 0xdd8d434c },
        { 0xffc00000, 0x90c8b228, 0xffc00000, 0xde0d534c },
        { 0xffc00000, 0x9148b228, 0xffc00000, 0xdecd5350 },
        { 0xffc00000, 0x91c8c22c, 0xffc00000, 0xdf8d6354 },
        { 0xffc00000, 0x9248d22c, 0xffc00000, 0xe08d7354 },
        { 0xffc00000, 0x92c8d230, 0xffc00000, 0xe1cd8358 },
        { 0xffc00000, 0x9388e234, 0xffc00000, 0xe30d935c },
        { 0xffc00000, 0xf648e234, 0xffc00000, 0xf64db364 },
        { 0xffc00000, 0xf648f238, 0xffc00000, 0xf64dc368 },
        { 0xffc00000, 0xf648f23c, 0xffc00000, 0xf64dd36c },
        { 0xffc00000, 0xf649023c, 0xffc00000, 0xf64de370 },
        { 0xffc00000, 0xf649123c, 0xffc00000, 0xf64df374 },
        { 0xffc00000, 0xf6492240, 0xffc00000, 0xf64e1378 },
    },
};
#else/* For 26 MHz PLL reference clock */
static const struct dptc_dcvr_table_entry
dptc_dcvr_table[DVFS_NUM_LEVELS][DPTC_NUM_WP] =
{
        /*  DCVR0       DCVR1       DCVR2       DCVR3    */
    {   /* 528 MHz */
        { 0xffc00000, 0x95c00000, 0xffc00000, 0xe5800000 },
        { 0xffc00000, 0x95e3e8e4, 0xffc00000, 0xe5b6fda0 },
        { 0xffc00000, 0x95e3e8e4, 0xffc00000, 0xe5b6fda0 },
        { 0xffc00000, 0x95e3e8e8, 0xffc00000, 0xe5f70da4 },
        { 0xffc00000, 0x9623f8e8, 0xffc00000, 0xe6371da8 },
        { 0xffc00000, 0x966408f0, 0xffc00000, 0xe6b73db0 },
        { 0xffc00000, 0x96e428f4, 0xffc00000, 0xe7776dbc },
        { 0xffc00000, 0x976448fc, 0xffc00000, 0xe8379dc8 },
        { 0xffc00000, 0x97e46904, 0xffc00000, 0xe977ddd8 },
        { 0xffc00000, 0x98a48910, 0xffc00000, 0xeab81de8 },
        { 0xffc00000, 0x9964b918, 0xffc00000, 0xebf86df8 },
        { 0xffc00000, 0xffe4e924, 0xffc00000, 0xfff8ae08 },
        { 0xffc00000, 0xffe5192c, 0xffc00000, 0xfff8fe1c },
        { 0xffc00000, 0xffe54938, 0xffc00000, 0xfff95e2c },
        { 0xffc00000, 0xffe57944, 0xffc00000, 0xfff9ae44 },
        { 0xffc00000, 0xffe5b954, 0xffc00000, 0xfffa0e58 },
        { 0xffc00000, 0xffe5e960, 0xffc00000, 0xfffa6e70 },
    },
    {   /* 264 MHz */
        { 0xffc00000, 0x95c00000, 0xffc00000, 0xe5800000 },
        { 0xffc00000, 0x95c8f238, 0xffc00000, 0xe58dc368 },
        { 0xffc00000, 0x95c8f238, 0xffc00000, 0xe58dc368 },
        { 0xffc00000, 0x95c8f238, 0xffc00000, 0xe5cdc368 },
        { 0xffc00000, 0x9609023c, 0xffc00000, 0xe60dc36c },
        { 0xffc00000, 0x9649023c, 0xffc00000, 0xe68dd36c },
        { 0xffc00000, 0x96c9023c, 0xffc00000, 0xe74dd370 },
        { 0xffc00000, 0x97491240, 0xffc00000, 0xe80de374 },
        { 0xffc00000, 0x97c92240, 0xffc00000, 0xe94df374 },
        { 0xffc00000, 0x98892244, 0xffc00000, 0xea8e0378 },
        { 0xffc00000, 0x99493248, 0xffc00000, 0xebce137c },
        { 0xffc00000, 0xffc93248, 0xffc00000, 0xffce3384 },
        { 0xffc00000, 0xffc9424c, 0xffc00000, 0xffce4388 },
        { 0xffc00000, 0xffc95250, 0xffc00000, 0xffce538c },
        { 0xffc00000, 0xffc96250, 0xffc00000, 0xffce7390 },
        { 0xffc00000, 0xffc97254, 0xffc00000, 0xffce8394 },
        { 0xffc00000, 0xffc98258, 0xffc00000, 0xffcea39c },
    },
    {   /* 132 MHz */
        { 0xffc00000, 0x95c00000, 0xffc00000, 0xe5800000 },
        { 0xffc00000, 0x95c8f238, 0xffc00000, 0xe58dc368 },
        { 0xffc00000, 0x95c8f238, 0xffc00000, 0xe58dc368 },
        { 0xffc00000, 0x95c8f238, 0xffc00000, 0xe5cdc368 },
        { 0xffc00000, 0x9609023c, 0xffc00000, 0xe60dc36c },
        { 0xffc00000, 0x9649023c, 0xffc00000, 0xe68dd36c },
        { 0xffc00000, 0x96c9023c, 0xffc00000, 0xe74dd370 },
        { 0xffc00000, 0x97491240, 0xffc00000, 0xe80de374 },
        { 0xffc00000, 0x97c92240, 0xffc00000, 0xe94df374 },
        { 0xffc00000, 0x98892244, 0xffc00000, 0xea8e0378 },
        { 0xffc00000, 0x99493248, 0xffc00000, 0xebce137c },
        { 0xffc00000, 0xffc93248, 0xffc00000, 0xffce3384 },
        { 0xffc00000, 0xffc9424c, 0xffc00000, 0xffce4388 },
        { 0xffc00000, 0xffc95250, 0xffc00000, 0xffce538c },
        { 0xffc00000, 0xffc96250, 0xffc00000, 0xffce7390 },
        { 0xffc00000, 0xffc97254, 0xffc00000, 0xffce8394 },
        { 0xffc00000, 0xffc98258, 0xffc00000, 0xffcea39c },
    },
};
#endif


/* For 27 MHz PLL reference clock */
static const struct dvfs_clock_table_entry
dvfs_clock_table[DVFS_NUM_LEVELS] =
{
    /*  PLL val    PDR0 val  PLL VSCNT */
    { 0x00082407, 0xff841e58, 1,   7 }, /* MCUPLL, 528 MHz, /1 = 528 MHz */
    { 0x00082407, 0xff841e59, 1,   7 }, /* MCUPLL, 528 MHz, /2 = 264 MHz */
    { 0x00082407, 0xff841e5b, 1,   7 }, /* MCUPLL, 528 MHz, /4 = 132 MHz */
};


/* DVFS load-tracking signal weights and detect modes */
static const struct dvfs_lt_signal_descriptor lt_signals[16] =
{
    { 0, 0 }, /* DVFS_LT_SIG_M3IF_M0_BUF */
    { 0, 0 }, /* DVFS_LT_SIG_M3IF_M1 */
    { 0, 0 }, /* DVFS_LT_SIG_MBX_MBXCLKGATE */
    { 0, 0 }, /* DVFS_LT_SIG_M3IF_M3 */
    { 0, 0 }, /* DVFS_LT_SIG_M3IF_M4 */
    { 0, 0 }, /* DVFS_LT_SIG_M3IF_M5 */
    { 0, 0 }, /* DVFS_LT_SIG_M3IF_M6 */
    { 0, 0 }, /* DVFS_LT_SIG_M3IF_M7 */
    { 0, 0 }, /* DVFS_LT_SIG_ARM11_P_IRQ_B_RBT_GATE */
    { 0, 0 }, /* DVFS_LT_SIG_ARM11_P_FIQ_B_RBT_GATE */
    { 0, 0 }, /* DVFS_LT_SIG_IPI_GPIO1_INT0 */
    { 0, 0 }, /* DVFS_LT_SIG_IPI_INT_IPU_FUNC */
    { 7, 0 }, /* DVFS_LT_SIG_DVGP0 */
    { 7, 0 }, /* DVFS_LT_SIG_DVGP1 */
    { 7, 0 }, /* DVFS_LT_SIG_DVGP2 */
    { 7, 0 }, /* DVFS_LT_SIG_DVGP3 */
};

#endif /* _DVFS_DPTC_TARGET_H_ */

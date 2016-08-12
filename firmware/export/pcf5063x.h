/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Philips PCF50633 Power Managemnt Unit (PMU) driver
 * (C) 2006-2007 by Openmoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
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
#ifndef PCF5063X_H
#define PCF5063X_H

enum pcf5063X_regs {
        PCF5063X_REG_VERSION    = 0x00,
        PCF5063X_REG_VARIANT    = 0x01,
        PCF5063X_REG_INT1       = 0x02, /* Interrupt Status */
        PCF5063X_REG_INT2       = 0x03, /* Interrupt Status */
        PCF5063X_REG_INT3       = 0x04, /* Interrupt Status */
        PCF5063X_REG_INT4       = 0x05, /* Interrupt Status */
        PCF5063X_REG_INT5       = 0x06, /* Interrupt Status */
        PCF5063X_REG_INT1M      = 0x07, /* Interrupt Mask */
        PCF5063X_REG_INT2M      = 0x08, /* Interrupt Mask */
        PCF5063X_REG_INT3M      = 0x09, /* Interrupt Mask */
        PCF5063X_REG_INT4M      = 0x0a, /* Interrupt Mask */
        PCF5063X_REG_INT5M      = 0x0b, /* Interrupt Mask */
        PCF5063X_REG_OOCSHDWN   = 0x0c,
        PCF5063X_REG_OOCWAKE    = 0x0d,
        PCF5063X_REG_OOCTIM1    = 0x0e,
        PCF5063X_REG_OOCTIM2    = 0x0f,
        PCF5063X_REG_OOCMODE    = 0x10,
        PCF5063X_REG_OOCCTL     = 0x11,
        PCF5063X_REG_OOCSTAT    = 0x12,
        PCF5063X_REG_GPIOCTL    = 0x13,
        PCF5063X_REG_GPIO1CFG   = 0x14,
        PCF5063X_REG_GPIO2CFG   = 0x15,
        PCF5063X_REG_GPIO3CFG   = 0x16,
        PCF5063X_REG_GPOCFG     = 0x17,
        PCF5063X_REG_BVMCTL     = 0x18,
        PCF5063X_REG_SVMCTL     = 0x19,
        PCF5063X_REG_AUTOOUT    = 0x1a,
        PCF5063X_REG_AUTOENA    = 0x1b,
        PCF5063X_REG_AUTOCTL    = 0x1c,
        PCF5063X_REG_AUTOMXC    = 0x1d,
        PCF5063X_REG_DOWN1OUT   = 0x1e,
        PCF5063X_REG_DOWN1ENA   = 0x1f,
        PCF5063X_REG_DOWN1CTL   = 0x20,
        PCF5063X_REG_DOWN1MXC   = 0x21,
        PCF5063X_REG_DOWN2OUT   = 0x22,
        PCF5063X_REG_DOWN2ENA   = 0x23,
        PCF5063X_REG_DOWN2CTL   = 0x24,
        PCF5063X_REG_DOWN2MXC   = 0x25,
        PCF5063X_REG_MEMLDOOUT  = 0x26,
        PCF5063X_REG_MEMLDOENA  = 0x27,
        PCF5063X_REG_LEDOUT     = 0x28,
        PCF5063X_REG_LEDENA     = 0x29,
        PCF5063X_REG_LEDCTL     = 0x2a,
        PCF5063X_REG_LEDDIM     = 0x2b,
        /* reserved */
        PCF5063X_REG_LDO1OUT    = 0x2d,
        PCF5063X_REG_LDO1ENA    = 0x2e,
        PCF5063X_REG_LDO2OUT    = 0x2f,
        PCF5063X_REG_LDO2ENA    = 0x30,
        PCF5063X_REG_LDO3OUT    = 0x31,
        PCF5063X_REG_LDO3ENA    = 0x32,
        PCF5063X_REG_LDO4OUT    = 0x33,
        PCF5063X_REG_LDO4ENA    = 0x34,
        PCF5063X_REG_LDO5OUT    = 0x35,
        PCF5063X_REG_LDO5ENA    = 0x36,
        PCF5063X_REG_LDO6OUT    = 0x37,
        PCF5063X_REG_LDO6ENA    = 0x38,
        PCF5063X_REG_HCLDOOUT   = 0x39,
        PCF5063X_REG_HCLDOENA   = 0x3a,
        PCF5063X_REG_STBYCTL1   = 0x3b,
        PCF5063X_REG_STBYCTL2   = 0x3c,
        PCF5063X_REG_DEBPF1     = 0x3d,
        PCF5063X_REG_DEBPF2     = 0x3e,
        PCF5063X_REG_DEBPF3     = 0x3f,
        PCF5063X_REG_HCLDOOVL   = 0x40,
        PCF5063X_REG_DCDCSTAT   = 0x41,
        PCF5063X_REG_LDOSTAT    = 0x42,
        PCF5063X_REG_MBCC1      = 0x43,
        PCF5063X_REG_MBCC2      = 0x44,
        PCF5063X_REG_MBCC3      = 0x45,
        PCF5063X_REG_MBCC4      = 0x46,
        PCF5063X_REG_MBCC5      = 0x47,
        PCF5063X_REG_MBCC6      = 0x48,
        PCF5063X_REG_MBCC7      = 0x49,
        PCF5063X_REG_MBCC8      = 0x4a,
        PCF5063X_REG_MBCS1      = 0x4b,
        PCF5063X_REG_MBCS2      = 0x4c,
        PCF5063X_REG_MBCS3      = 0x4d,
        PCF5063X_REG_BBCCTL     = 0x4e,
        PCF5063X_REG_ALMGAIN    = 0x4f,
        PCF5063X_REG_ALMDATA    = 0x50,
        /* reserved */
        PCF5063X_REG_ADCC3      = 0x52,
        PCF5063X_REG_ADCC2      = 0x53,
        PCF5063X_REG_ADCC1      = 0x54,
        PCF5063X_REG_ADCS1      = 0x55,
        PCF5063X_REG_ADCS2      = 0x56,
        PCF5063X_REG_ADCS3      = 0x57,
        /* reserved */
        PCF5063X_REG_RTCSC      = 0x59, /* Second */
        PCF5063X_REG_RTCMN      = 0x5a, /* Minute */
        PCF5063X_REG_RTCHR      = 0x5b, /* Hour */
        PCF5063X_REG_RTCWD      = 0x5c, /* Weekday */
        PCF5063X_REG_RTCDT      = 0x5d, /* Day */
        PCF5063X_REG_RTCMT      = 0x5e, /* Month */
        PCF5063X_REG_RTCYR      = 0x5f, /* Year */
        PCF5063X_REG_RTCSCA     = 0x60, /* Alarm Second */
        PCF5063X_REG_RTCMNA     = 0x61, /* Alarm Minute */
        PCF5063X_REG_RTCHRA     = 0x62, /* Alarm Hour */
        PCF5063X_REG_RTCWDA     = 0x63, /* Alarm Weekday */
        PCF5063X_REG_RTCDTA     = 0x64, /* Alarm Day */
        PCF5063X_REG_RTCMTA     = 0x65, /* Alarm Month */
        PCF5063X_REG_RTCYRA     = 0x66, /* Alarm Year */

        PCF5063X_REG_MEMBYTE0   = 0x67,
        PCF5063X_REG_MEMBYTE1   = 0x68,
        PCF5063X_REG_MEMBYTE2   = 0x69,
        PCF5063X_REG_MEMBYTE3   = 0x6a,
        PCF5063X_REG_MEMBYTE4   = 0x6b,
        PCF5063X_REG_MEMBYTE5   = 0x6c,
        PCF5063X_REG_MEMBYTE6   = 0x6d,
        PCF5063X_REG_MEMBYTE7   = 0x6e,
        /* reserved */
        PCF5063X_REG_DCDCPFM    = 0x84,
        __NUM_PCF5063X_REGS
};


enum pcf5063X_reg_oocshdwn {
        PCF5063X_OOCSHDWN_GOSTDBY       = 0x01,
        PCF5063X_OOCSHDWN_TOTRST        = 0x04,
        PCF5063X_OOCSHDWN_COLDBOOT      = 0x08,
};

enum pcf5063X_reg_oocwake {
        PCF5063X_OOCWAKE_ONKEY          = 0x01,
        PCF5063X_OOCWAKE_EXTON1         = 0x02,
        PCF5063X_OOCWAKE_EXTON2         = 0x04,
        PCF5063X_OOCWAKE_EXTON3         = 0x08,
        PCF5063X_OOCWAKE_RTC            = 0x10,
        /* reserved */
        PCF5063X_OOCWAKE_USB            = 0x40,
        PCF5063X_OOCWAKE_ADP            = 0x80,
};

enum pcf5063X_reg_oocstat {
        PCF5063X_OOCSTAT_ONKEY          = 0x01,
        PCF5063X_OOCSTAT_EXTON1         = 0x02,
        PCF5063X_OOCSTAT_EXTON2         = 0x04,
        PCF5063X_OOCSTAT_EXTON3         = 0x08,
        PCF5063X_OOCSTAT_BUBPRES        = 0x10,
        PCF5063X_OOCSTAT_SYSOK          = 0x20,
        PCF5063X_OOCSTAT_BATOK          = 0x40,
        PCF5063X_OOCSTAT_TMPOK          = 0x80,
};

enum pcf5063X_reg_mbcc1 {
        PCF5063X_MBCC1_CHGENA           = 0x01, /* Charger enable */
        PCF5063X_MBCC1_AUTOSTOP         = 0x02,
        PCF5063X_MBCC1_AUTORES          = 0x04, /* automatic resume */
        PCF5063X_MBCC1_RESUME           = 0x08, /* explicit resume cmd */
        PCF5063X_MBCC1_RESTART          = 0x10, /* restart charging */
        PCF5063X_MBCC1_PREWDTIME_60M    = 0x20, /* max. precharging time */
        PCF5063X_MBCC1_WDTIME_1H        = 0x00,
        PCF5063X_MBCC1_WDTIME_2H        = 0x40,
        PCF5063X_MBCC1_WDTIME_4H        = 0x80,
        PCF5063X_MBCC1_WDTIME_6H        = 0xc0,
};
#define PCF5063X_MBCC1_WDTIME_MASK        0xc0

enum pcf5063X_reg_mbcc2 {
        PCF5063X_MBCC2_VBATCOND_2V7     = 0x00,
        PCF5063X_MBCC2_VBATCOND_2V85    = 0x01,
        PCF5063X_MBCC2_VBATCOND_3V0     = 0x02,
        PCF5063X_MBCC2_VBATCOND_3V15    = 0x03,
        PCF5063X_MBCC2_VMAX_4V          = 0x00,
        PCF5063X_MBCC2_VMAX_4V20        = 0x28,
        PCF5063X_MBCC2_VRESDEBTIME_64S  = 0x80, /* debounce time (32/64sec) */
};
#define PCF5063X_MBCC2_VBATCOND_MASK      0x03
#define PCF5063X_MBCC2_VMAX_MASK          0x3c

enum pcf5063X_reg_adcc1 {
        PCF5063X_ADCC1_ADCSTART         = 0x01,
        PCF5063X_ADCC1_RES_10BIT        = 0x00,
        PCF5063X_ADCC1_RES_8BIT         = 0x02,
        PCF5063X_ADCC1_AVERAGE_NO       = 0x00,
        PCF5063X_ADCC1_AVERAGE_4        = 0x04,
        PCF5063X_ADCC1_AVERAGE_8        = 0x08,
        PCF5063X_ADCC1_AVERAGE_16       = 0x0c,
        PCF5063X_ADCC1_MUX_BATSNS_RES   = 0x00,
        PCF5063X_ADCC1_MUX_BATSNS_SUBTR = 0x10,
        PCF5063X_ADCC1_MUX_ADCIN2_RES   = 0x20,
        PCF5063X_ADCC1_MUX_ADCIN2_SUBTR = 0x30,
        PCF5063X_ADCC1_MUX_BATTEMP      = 0x60,
        PCF5063X_ADCC1_MUX_ADCIN1       = 0x70,
};
#define PCF5063X_ADCC1_RES_MASK         0x02
#define PCF5063X_ADCC1_AVERAGE_MASK     0x0c
#define PCF5063X_ADCC1_ADCMUX_MASK      0xf0

enum pcf5063X_reg_adcc2 {
        PCF5063X_ADCC2_RATIO_NONE       = 0x00,
        PCF5063X_ADCC2_RATIO_BATTEMP    = 0x01,
        PCF5063X_ADCC2_RATIO_ADCIN1     = 0x02,
        PCF5063X_ADCC2_RATIO_BOTH       = 0x03,
        PCF5063X_ADCC2_RATIOSETTL_10US  = 0x00,
        PCF5063X_ADCC2_RATIOSETTL_100US = 0x04,
};
#define PCF5063X_ADCC2_RATIO_MASK       0x03
#define PCF5063X_ADCC2_RATIOSETTL_MASK  0x04

enum pcf5063X_reg_adcc3 {
        PCF5063X_ADCC3_ACCSW_EN         = 0x01,
        PCF5063X_ADCC3_NTCSW_EN         = 0x04,
        PCF5063X_ADCC3_RES_DIV_TWO      = 0x10,
        PCF5063X_ADCC3_RES_DIV_THREE    = 0x00,
};
#define PCF5063X_ADCC3_RES_DIV_MASK     0x10

enum pcf5063X_reg_adcs3 {
        PCF5063X_ADCS3_REF_NTCSW        = 0x00,
        PCF5063X_ADCS3_REF_ACCSW        = 0x10,
        PCF5063X_ADCS3_REF_2V0          = 0x20,
        PCF5063X_ADCS3_REF_VISA         = 0x30,
        PCF5063X_ADCS3_REF_2V0_2        = 0x70,
        PCF5063X_ADCS3_ADCRDY           = 0x80,
};
#define PCF5063X_ADCS3_ADCDAT1L_MASK    0x03
#define PCF5063X_ADCS3_ADCDAT2L_MASK    0x0c
#define PCF5063X_ADCS3_ADCDAT2L_SHIFT   2
#define PCF5063X_ASCS3_REF_MASK         0x70

enum pcf5063X_regulator_enable {
        PCF5063X_REGULATOR_ON           = 0x01,
        PCF5063X_REGULATOR_ON_GPIO1     = 0x02,
        PCF5063X_REGULATOR_ON_GPIO2     = 0x04,
        PCF5063X_REGULATOR_ON_GPIO3     = 0x08,
};
#define PCF5063X_REGULATOR_ON_MASK      0x0f

enum pcf5063X_regulator_phase {
        PCF5063X_REGULATOR_ACTPH1       = 0x00,
        PCF5063X_REGULATOR_ACTPH2       = 0x10,
        PCF5063X_REGULATOR_ACTPH3       = 0x20,
        PCF5063X_REGULATOR_ACTPH4       = 0x30,
};
#define PCF5063X_REGULATOR_ACTPH_MASK   0x30

enum pcf5063X_reg_gpocfg {
        PCF5063X_GPOCFG_GPOSEL_0        = 0x00,
        PCF5063X_GPOCFG_GPOSEL_LED_NFET = 0x01,
        PCF5063X_GPOCFG_GPOSEL_SYSxOK   = 0x02,
        PCF5063X_GPOCFG_GPOSEL_CLK32K   = 0x03,
        PCF5063X_GPOCFG_GPOSEL_ADAPUSB  = 0x04,
        PCF5063X_GPOCFG_GPOSEL_USBxOK   = 0x05,
        PCF5063X_GPOCFG_GPOSEL_ACTPH4   = 0x06,
        PCF5063X_GPOCFG_GPOSEL_1        = 0x07,
        PCF5063X_GPOCFG_GPOSEL_INVERSE  = 0x08,
};
#define PCF5063X_GPOCFG_GPOSEL_MASK     0x07

enum pcf5063X_reg_mbcc7 {
        PCF5063X_MBCC7_USB_100mA        = 0x00,
        PCF5063X_MBCC7_USB_500mA        = 0x01,
        PCF5063X_MBCC7_USB_1000mA       = 0x02,
        PCF5063X_MBCC7_USB_SUSPEND      = 0x03,
        PCF5063X_MBCC7_BATTEMP_EN       = 0x04,
        PCF5063X_MBCC7_BATSYSIMAX_1A6   = 0x00,
        PCF5063X_MBCC7_BATSYSIMAX_1A8   = 0x40,
        PCF5063X_MBCC7_BATSYSIMAX_2A0   = 0x80,
        PCF5063X_MBCC7_BATSYSIMAX_2A2   = 0xc0,
};
#define PCF56033_MBCC7_USB_MASK         0x03

enum pcf5063X_reg_mbcc8 {
        PCF5063X_MBCC8_USBENASUS        = 0x10,
};

enum pcf5063X_reg_mbcs1 {
        PCF5063X_MBCS1_USBPRES          = 0x01,
        PCF5063X_MBCS1_USBOK            = 0x02,
        PCF5063X_MBCS1_ADAPTPRES        = 0x04,
        PCF5063X_MBCS1_ADAPTOK          = 0x08,
        PCF5063X_MBCS1_TBAT_OK          = 0x00,
        PCF5063X_MBCS1_TBAT_ABOVE       = 0x10,
        PCF5063X_MBCS1_TBAT_BELOW       = 0x20,
        PCF5063X_MBCS1_TBAT_UNDEF       = 0x30,
        PCF5063X_MBCS1_PREWDTEXP        = 0x40,
        PCF5063X_MBCS1_WDTEXP           = 0x80,
};

enum pcf5063X_reg_mbcs2_mbcmod {
        PCF5063X_MBCS2_MBC_PLAY         = 0x00,
        PCF5063X_MBCS2_MBC_USB_PRE      = 0x01,
        PCF5063X_MBCS2_MBC_USB_PRE_WAIT = 0x02,
        PCF5063X_MBCS2_MBC_USB_FAST     = 0x03,
        PCF5063X_MBCS2_MBC_USB_FAST_WAIT= 0x04,
        PCF5063X_MBCS2_MBC_USB_SUSPEND  = 0x05,
        PCF5063X_MBCS2_MBC_ADP_PRE      = 0x06,
        PCF5063X_MBCS2_MBC_ADP_PRE_WAIT = 0x07,
        PCF5063X_MBCS2_MBC_ADP_FAST     = 0x08,
        PCF5063X_MBCS2_MBC_ADP_FAST_WAIT= 0x09,
        PCF5063X_MBCS2_MBC_BAT_FULL     = 0x0a,
        PCF5063X_MBCS2_MBC_HALT         = 0x0b,
};
#define PCF5063X_MBCS2_MBC_MASK         0x0f
enum pcf5063X_reg_mbcs2_chgstat {
        PCF5063X_MBCS2_CHGS_NONE        = 0x00,
        PCF5063X_MBCS2_CHGS_ADAPTER     = 0x10,
        PCF5063X_MBCS2_CHGS_USB         = 0x20,
        PCF5063X_MBCS2_CHGS_BOTH        = 0x30,
};
#define PCF5063X_MBCS2_RESSTAT_AUTO     0x40

enum pcf5063X_reg_mbcs3 {
        PCF5063X_MBCS3_USBLIM_PLAY      = 0x01,
        PCF5063X_MBCS3_USBLIM_CGH       = 0x02,
        PCF5063X_MBCS3_TLIM_PLAY        = 0x04,
        PCF5063X_MBCS3_TLIM_CHG         = 0x08,
        PCF5063X_MBCS3_ILIM             = 0x10, /* 1: Ibat > Icutoff */
        PCF5063X_MBCS3_VLIM             = 0x20, /* 1: Vbat == Vmax */
        PCF5063X_MBCS3_VBATSTAT         = 0x40, /* 1: Vbat > Vbatcond */
        PCF5063X_MBCS3_VRES             = 0x80, /* 1: Vbat > Vth(RES) */
};


enum pcf5063X_reg_int1 {
        PCF5063X_INT1_ADPINS    = 0x01, /* Adapter inserted */
        PCF5063X_INT1_ADPREM    = 0x02, /* Adapter removed */
        PCF5063X_INT1_USBINS    = 0x04, /* USB inserted */
        PCF5063X_INT1_USBREM    = 0x08, /* USB removed */
        /* reserved */
        PCF5063X_INT1_ALARM     = 0x40, /* RTC alarm time is reached */
        PCF5063X_INT1_SECOND    = 0x80, /* RTC periodic second interrupt */
};

enum pcf5063X_reg_int2 {
        PCF5063X_INT2_ONKEYR    = 0x01, /* ONKEY rising edge */
        PCF5063X_INT2_ONKEYF    = 0x02, /* ONKEY falling edge */
        PCF5063X_INT2_EXTON1R   = 0x04, /* EXTON1 rising edge */
        PCF5063X_INT2_EXTON1F   = 0x08, /* EXTON1 falling edge */
        PCF5063X_INT2_EXTON2R   = 0x10, /* EXTON2 rising edge */
        PCF5063X_INT2_EXTON2F   = 0x20, /* EXTON2 falling edge */
        PCF5063X_INT2_EXTON3R   = 0x40, /* EXTON3 rising edge */
        PCF5063X_INT2_EXTON3F   = 0x80, /* EXTON3 falling edge */
};

enum pcf5063X_reg_int3 {
        PCF5063X_INT3_BATFULL   = 0x01, /* Battery full */
        PCF5063X_INT3_CHGHALT   = 0x02, /* Charger halt */
        PCF5063X_INT3_THLIMON   = 0x04,
        PCF5063X_INT3_THLIMOFF  = 0x08,
        PCF5063X_INT3_USBLIMON  = 0x10,
        PCF5063X_INT3_USBLIMOFF = 0x20,
        PCF5063X_INT3_ADCRDY    = 0x40, /* ADC result ready */
        PCF5063X_INT3_ONKEY1S   = 0x80, /* ONKEY pressed 1 second */
};

enum pcf5063X_reg_int4 {
        PCF5063X_INT4_LOWSYS            = 0x01,
        PCF5063X_INT4_LOWBAT            = 0x02,
        PCF5063X_INT4_HIGHTMP           = 0x04,
        PCF5063X_INT4_AUTOPWRFAIL       = 0x08,
        PCF5063X_INT4_DWN1PWRFAIL       = 0x10,
        PCF5063X_INT4_DWN2PWRFAIL       = 0x20,
        PCF5063X_INT4_LEDPWRFAIL        = 0x40,
        PCF5063X_INT4_LEDOVP            = 0x80,
};

enum pcf5063X_reg_int5 {
        PCF5063X_INT5_LDO1PWRFAIL       = 0x01,
        PCF5063X_INT5_LDO2PWRFAIL       = 0x02,
        PCF5063X_INT5_LDO3PWRFAIL       = 0x04,
        PCF5063X_INT5_LDO4PWRFAIL       = 0x08,
        PCF5063X_INT5_LDO5PWRFAIL       = 0x10,
        PCF5063X_INT5_LDO6PWRFAIL       = 0x20,
        PCF5063X_INT5_HCLDOPWRFAIL      = 0x40,
        PCF5063X_INT5_HCLDOOVL          = 0x80,
};


#endif /* PCF5063X_H */

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: wm8975.h 28159 2010-09-24 22:42:06Z Buschel $
 *
 * Copyright (C) 2010 by Michael Sparmann
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

#ifndef __CS42L55_H__
#define __CS42L55_H__

/* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -580
#define VOLUME_MAX  120

#define AUDIOHW_CAPS (BASS_CAP | TREBLE_CAP)

extern int tenthdb2master(int db);

extern void audiohw_set_master_vol(int vol_l, int vol_r);
extern void audiohw_set_lineout_vol(int vol_l, int vol_r);
extern void audiohw_enable_lineout(bool enable);

/* Register addresses and bits */

#define HIDDENCTL               0x00
#define HIDDENCTL_LOCK          0x00
#define HIDDENCTL_UNLOCK        0x99

#define CHIPVERSION             0x01

#define PWRCTL1                 0x02
#define PWRCTL1_PDN_CODEC       (1 << 0)
#define PWRCTL1_PDN_ADCA        (1 << 1)
#define PWRCTL1_PDN_ADCB        (1 << 2)
#define PWRCTL1_PDN_CHRG        (1 << 3)

#define PWRCTL2                 0x03
#define PWRCTL2_PDN_LINA_MASK   (3 << 0)
#define PWRCTL2_PDN_LINA_HIGH   (0 << 0)
#define PWRCTL2_PDN_LINA_LOW    (1 << 0)
#define PWRCTL2_PDN_LINA_NEVER  (2 << 0)
#define PWRCTL2_PDN_LINA_ALWAYS (3 << 0)
#define PWRCTL2_PDN_LINB_MASK   (3 << 2)
#define PWRCTL2_PDN_LINB_HIGH   (0 << 2)
#define PWRCTL2_PDN_LINB_LOW    (1 << 2)
#define PWRCTL2_PDN_LINB_NEVER  (2 << 2)
#define PWRCTL2_PDN_LINB_ALWAYS (3 << 2)
#define PWRCTL2_PDN_HPA_MASK    (3 << 4)
#define PWRCTL2_PDN_HPA_HIGH    (0 << 4)
#define PWRCTL2_PDN_HPA_LOW     (1 << 4)
#define PWRCTL2_PDN_HPA_NEVER   (2 << 4)
#define PWRCTL2_PDN_HPA_ALWAYS  (3 << 4)
#define PWRCTL2_PDN_HPB_MASK    (3 << 6)
#define PWRCTL2_PDN_HPB_HIGH    (0 << 6)
#define PWRCTL2_PDN_HPB_LOW     (1 << 6)
#define PWRCTL2_PDN_HPB_NEVER   (2 << 6)
#define PWRCTL2_PDN_HPB_ALWAYS  (3 << 6)

#define CLKCTL1                 0x04
#define CLKCTL1_MCLKDIS         (1 << 0)
#define CLKCTL1_MCLKDIV2        (1 << 1)
#define CLKCTL1_SCLKMCLK_MASK   (3 << 2)
#define CLKCTL1_SCLKMCLK_BURST  (0 << 2)
#define CLKCTL1_SCLKMCLK_AFTER  (2 << 2)
#define CLKCTL1_SCLKMCLK_BEFORE (3 << 2)
#define CLKCTL1_INV_SCLK        (1 << 4)
#define CLKCTL1_MASTER          (1 << 5)

#define CLKCTL2                 0x05
#define CLKCTL2_MCLKLRCK_MASK   (3 << 0)
#define CLKCTL2_MCLKLRCK_125    (1 << 0)
#define CLKCTL2_MCLKLRCK_136    (3 << 0)
#define CLKCTL2_32KGROUP        (1 << 2)
#define CLKCTL2_SPEED_MASK      (3 << 3)
#define CLKCTL2_SPEED_SINGLE    (1 << 3)
#define CLKCTL2_SPEED_HALF      (2 << 3)
#define CLKCTL2_SPEED_QUARTER   (3 << 3)
#define CLKCTL2_8000HZ          0x1d
#define CLKCTL2_11025HZ         0x1b
#define CLKCTL2_12000HZ         0x19
#define CLKCTL2_16000HZ         0x15
#define CLKCTL2_22050HZ         0x13
#define CLKCTL2_24000HZ         0x11
#define CLKCTL2_32000HZ         0x0d
#define CLKCTL2_44100HZ         0x0b
#define CLKCTL2_48000HZ         0x09

#define CLSHCTL                 0x06
#define CLSHCTL_ADPTPWR_MASK    (3 << 4)
#define CLSHCTL_ADPTPWR_VOLUME  (0 << 4)
#define CLSHCTL_ADPTPWR_HALF    (1 << 4)
#define CLSHCTL_ADPTPWR_FULL    (2 << 4)
#define CLSHCTL_ADPTPWR_SIGNAL  (3 << 4)

#define MISCCTL                 0x07
#define MISCCTL_FREEZE          (1 << 0)
#define MISCCTL_DIGSFT          (1 << 2)
#define MISCCTL_ANLGZC          (1 << 3)
#define MISCCTL_UNDOC4          (1 << 4)
#define MISCCTL_DIGMUX          (1 << 7)

#define ALHMUX                  0x08
#define ALHMUX_HPAMUX_MASK      (1 << 0)
#define ALHMUX_HPAMUX_DACA      (0 << 0)
#define ALHMUX_HPAMUX_PGAA      (1 << 0)
#define ALHMUX_HPBMUX_MASK      (1 << 1)
#define ALHMUX_HPBMUX_DACB      (0 << 1)
#define ALHMUX_HPBMUX_PGAB      (1 << 1)
#define ALHMUX_LINEAMUX_MASK    (1 << 2)
#define ALHMUX_LINEAMUX_DACA    (0 << 2)
#define ALHMUX_LINEAMUX_PGAA    (1 << 2)
#define ALHMUX_LINEBMUX_MASK    (1 << 3)
#define ALHMUX_LINEBMUX_DACB    (0 << 3)
#define ALHMUX_LINEBMUX_PGAB    (1 << 3)
#define ALHMUX_ADCAMUX_MASK     (3 << 4)
#define ALHMUX_ADCAMUX_PGAA     (0 << 4)
#define ALHMUX_ADCAMUX_AIN1A    (1 << 4)
#define ALHMUX_ADCAMUX_AIN2A    (2 << 4)
#define ALHMUX_ADCBMUX_MASK     (3 << 4)
#define ALHMUX_ADCBMUX_PGAB     (0 << 6)
#define ALHMUX_ADCBMUX_AIN1B    (1 << 6)
#define ALHMUX_ADCBMUX_AIN2B    (2 << 6)

#define HPFCTL                  0x09
#define HPFCTL_HPFA_CF_MASK     (3 << 0)
#define HPFCTL_HPFA_CF_1_8      (0 << 0)
#define HPFCTL_HPFA_CF_119      (1 << 0)
#define HPFCTL_HPFA_CF_236      (2 << 0)
#define HPFCTL_HPFA_CF_464      (3 << 0)
#define HPFCTL_HPFB_CF_MASK     (3 << 2)
#define HPFCTL_HPFB_CF_1_8      (0 << 2)
#define HPFCTL_HPFB_CF_119      (1 << 2)
#define HPFCTL_HPFB_CF_236      (2 << 2)
#define HPFCTL_HPFB_CF_464      (3 << 2)
#define HPFCTL_HPFRZA           (1 << 4)
#define HPFCTL_HPFA             (1 << 5)
#define HPFCTL_HPFRZB           (1 << 6)
#define HPFCTL_HPFB             (1 << 7)

#define ADCCTL                  0x0a
#define ADCCTL_ADCAMUTE         (1 << 0)
#define ADCCTL_ADCBMUTE         (1 << 1)
#define ADCCTL_INV_ADCA         (1 << 2)
#define ADCCTL_INV_ADCB         (1 << 3)
#define ADCCTL_DIGSUM_MASK      (3 << 4)
#define ADCCTL_DIGSUM_NORMAL    (0 << 4)
#define ADCCTL_DIGSUM_HALFSUM   (1 << 4)
#define ADCCTL_DIGSUM_HALFDIFF  (2 << 4)
#define ADCCTL_DIGSUM_SWAPPED   (3 << 4)
#define ADCCTL_PGA_VOLUME_GROUP (1 << 6)
#define ADCCTL_ADC_VOLUME_GROUP (1 << 7)

#define PGAACTL                 0x0b
#define PGAACTL_VOLUME_MASK     (0x3f << 0)
#define PGAACTL_VOLUME_SHIFT    0
#define PGAACTL_MUX_MASK        (1 << 6)
#define PGAACTL_MUX_AIN1A       (0 << 6)
#define PGAACTL_MUX_AIN2A       (1 << 6)
#define PGAACTL_BOOST           (1 << 7)

#define PGABCTL                 0x0c
#define PGABCTL_VOLUME_MASK     (0x3f << 0)
#define PGABCTL_VOLUME_SHIFT    0
#define PGABCTL_MUX_MASK        (1 << 6)
#define PGABCTL_MUX_AIN1B       (0 << 6)
#define PGABCTL_MUX_AIN2B       (1 << 6)
#define PGABCTL_BOOST           (1 << 7)

#define ADCAATT                 0x0d
#define ADCAATT_VOLUME_MASK     (0xff << 0)
#define ADCAATT_VOLUME_SHIFT    0

#define ADCBATT                 0x0e
#define ADCBATT_VOLUME_MASK     (0xff << 0)
#define ADCBATT_VOLUME_SHIFT    0

#define PLAYCTL                 0x0f
#define PLAYCTL_MSTAMUTE        (1 << 0)
#define PLAYCTL_MSTBMUTE        (1 << 1)
#define PLAYCTL_INV_PCMA        (1 << 2)
#define PLAYCTL_INV_PCMB        (1 << 3)
#define PLAYCTL_PB_VOLUME_GROUP (1 << 4)
#define PLAYCTL_DEEMPH          (1 << 6)
#define PLAYCTL_PDN_DSP         (1 << 7)

#define AMIXACTL                0x10
#define AMIXACTL_AMIXAVOL_MASK  (0x7f << 0)
#define AMIXACTL_AMIXAVOL_SHIFT 0
#define AMIXACTL_AMIXAMUTE      (1 << 7)

#define AMIXBCTL                0x11
#define AMIXBCTL_AMIXBVOL_MASK  (0x7f << 0)
#define AMIXBCTL_AMIXBVOL_SHIFT 0
#define AMIXBCTL_AMIXBMUTE      (1 << 7)

#define PMIXACTL                0x12
#define PMIXACTL_PMIXAVOL_MASK  (0x7f << 0)
#define PMIXACTL_PMIXAVOL_SHIFT 0
#define PMIXACTL_PMIXAMUTE      (1 << 7)

#define PMIXBCTL                0x13
#define PMIXBCTL_PMIXBVOL_MASK  (0x7f << 0)
#define PMIXBCTL_PMIXBVOL_SHIFT 0
#define PMIXBCTL_PMIXBMUTE      (1 << 7)

#define BEEPFO                  0x14
#define BEEPFO_ONTIME_MASK      (0xf << 0)
#define BEEPFO_ONTIME_86        (0x0 << 0)
#define BEEPFO_ONTIME_430       (0x1 << 0)
#define BEEPFO_ONTIME_780       (0x2 << 0)
#define BEEPFO_ONTIME_1200      (0x3 << 0)
#define BEEPFO_ONTIME_1500      (0x4 << 0)
#define BEEPFO_ONTIME_1800      (0x5 << 0)
#define BEEPFO_ONTIME_2200      (0x6 << 0)
#define BEEPFO_ONTIME_2500      (0x7 << 0)
#define BEEPFO_ONTIME_2800      (0x8 << 0)
#define BEEPFO_ONTIME_3200      (0x9 << 0)
#define BEEPFO_ONTIME_3500      (0xa << 0)
#define BEEPFO_ONTIME_3800      (0xb << 0)
#define BEEPFO_ONTIME_4200      (0xc << 0)
#define BEEPFO_ONTIME_4500      (0xd << 0)
#define BEEPFO_ONTIME_4800      (0xe << 0)
#define BEEPFO_ONTIME_5200      (0xf << 0)
#define BEEPFO_FREQ_MASK        (0xf << 4)
#define BEEPFO_FREQ_254_76      (0x0 << 4)
#define BEEPFO_FREQ_509_51      (0x1 << 4)
#define BEEPFO_FREQ_571_65      (0x2 << 4)
#define BEEPFO_FREQ_651_04      (0x3 << 4)
#define BEEPFO_FREQ_689_34      (0x4 << 4)
#define BEEPFO_FREQ_756_04      (0x5 << 4)
#define BEEPFO_FREQ_869_45      (0x6 << 4)
#define BEEPFO_FREQ_976_56      (0x7 << 4)
#define BEEPFO_FREQ_1019_02     (0x8 << 4)
#define BEEPFO_FREQ_1171_88     (0x9 << 4)
#define BEEPFO_FREQ_1302_08     (0xa << 4)
#define BEEPFO_FREQ_1378_67     (0xb << 4)
#define BEEPFO_FREQ_1562_50     (0xc << 4)
#define BEEPFO_FREQ_1674_11     (0xd << 4)
#define BEEPFO_FREQ_1953_13     (0xe << 4)
#define BEEPFO_FREQ_2130_68     (0xf << 4)

#define BEEPVO                  0x15
#define BEEPVO_VOLUME_MASK      (0x1f << 0)
#define BEEPVO_VOLUME_SHIFT     0
#define BEEPVO_OFFTIME_MASK     (7 << 5)
#define BEEPVO_OFFTIME_1230     (0 << 5)
#define BEEPVO_OFFTIME_2580     (1 << 5)
#define BEEPVO_OFFTIME_3900     (2 << 5)
#define BEEPVO_OFFTIME_5200     (3 << 5)
#define BEEPVO_OFFTIME_6600     (4 << 5)
#define BEEPVO_OFFTIME_8050     (5 << 5)
#define BEEPVO_OFFTIME_9350     (6 << 5)
#define BEEPVO_OFFTIME_10800    (7 << 5)

#define BTCTL                   0x16
#define BTCTL_TCEN              (1 << 0)
#define BTCTL_BASSCF_MASK       (3 << 1)
#define BTCTL_BASSCF_50         (0 << 1)
#define BTCTL_BASSCF_100        (1 << 1)
#define BTCTL_BASSCF_200        (2 << 1)
#define BTCTL_BASSCF_250        (3 << 1)
#define BTCTL_TREBCF_MASK       (3 << 3)
#define BTCTL_TREBCF_5000       (0 << 3)
#define BTCTL_TREBCF_7000       (1 << 3)
#define BTCTL_TREBCF_10000      (2 << 3)
#define BTCTL_TREBCF_15000      (3 << 3)
#define BTCTL_BEEP_MASK         (0 << 6)
#define BTCTL_BEEP_OFF          (0 << 6)
#define BTCTL_BEEP_SINGLE       (1 << 6)
#define BTCTL_BEEP_MULTIPLE     (2 << 6)
#define BTCTL_BEEP_CONTINUOUS   (3 << 6)

#define TONECTL                 0x17
#define TONECTL_BASS_MASK       (0xf << 0)
#define TONECTL_BASS_SHIFT      0
#define TONECTL_TREB_MASK       (0xf << 4)
#define TONECTL_TREB_SHIFT      4

#define MSTAVOL                 0x18
#define MSTAVOL_VOLUME_MASK     (0xff << 0)
#define MSTAVOL_VOLUME_SHIFT    0

#define MSTBVOL                 0x19
#define MSTBVOL_VOLUME_MASK     (0xff << 0)
#define MSTBVOL_VOLUME_SHIFT    0

#define HPACTL                  0x1a
#define HPACTL_HPAVOL_MASK      (0x7f << 0)
#define HPACTL_HPAVOL_SHIFT     0
#define HPACTL_HPAMUTE          (1 << 7)

#define HPBCTL                  0x1b
#define HPBCTL_HPBVOL_MASK      (0x7f << 0)
#define HPBCTL_HPBVOL_SHIFT     0
#define HPBCTL_HPBMUTE          (1 << 7)

#define LINEACTL                0x1c
#define LINEACTL_LINEAVOL_MASK  (0x7f << 0)
#define LINEACTL_LINEAVOL_SHIFT 0
#define LINEACTL_LINEAMUTE      (1 << 7)

#define LINEBCTL                0x1d
#define LINEBCTL_LINEBVOL_MASK  (0x7f << 0)
#define LINEBCTL_LINEBVOL_SHIFT 0
#define LINEBCTL_LINEBMUTE      (1 << 7)

#define AINADV                  0x1e
#define AINADV_VOLUME_MASK      (0xff << 0)
#define AINADV_VOLUME_SHIFT     0

#define DINADV                  0x1f
#define DINADV_VOLUME_MASK      (0xff << 0)
#define DINADV_VOLUME_SHIFT     0

#define MIXCTL                  0x20
#define MIXCTL_ADCASWP_MASK     (3 << 0)
#define MIXCTL_ADCASWP_NORMAL   (0 << 0)
#define MIXCTL_ADCASWP_HALFSUM  (1 << 0)
#define MIXCTL_ADCASWP_HALFSUM2 (2 << 0)
#define MIXCTL_ADCASWP_SWAPPED  (3 << 0)
#define MIXCTL_ADCBSWP_MASK     (3 << 2)
#define MIXCTL_ADCBSWP_NORMAL   (0 << 2)
#define MIXCTL_ADCBSWP_HALFSUM  (1 << 2)
#define MIXCTL_ADCBSWP_HALFSUM2 (2 << 2)
#define MIXCTL_ADCBSWP_SWAPPED  (3 << 2)
#define MIXCTL_PCMASWP_MASK     (3 << 4)
#define MIXCTL_PCMASWP_NORMAL   (0 << 4)
#define MIXCTL_PCMASWP_HALFSUM  (1 << 4)
#define MIXCTL_PCMASWP_HALFSUM2 (2 << 4)
#define MIXCTL_PCMASWP_SWAPPED  (3 << 4)
#define MIXCTL_PCMBSWP_MASK     (3 << 6)
#define MIXCTL_PCMBSWP_NORMAL   (0 << 6)
#define MIXCTL_PCMBSWP_HALFSUM  (1 << 6)
#define MIXCTL_PCMBSWP_HALFSUM2 (2 << 6)
#define MIXCTL_PCMBSWP_SWAPPED  (3 << 6)

#define LIMCTL1                 0x21
#define LIMCTL1_CUSH_MASK       (7 << 2)
#define LIMCTL1_CUSH_0          (0 << 2)
#define LIMCTL1_CUSH_3          (1 << 2)
#define LIMCTL1_CUSH_6          (2 << 2)
#define LIMCTL1_CUSH_9          (3 << 2)
#define LIMCTL1_CUSH_12         (4 << 2)
#define LIMCTL1_CUSH_18         (5 << 2)
#define LIMCTL1_CUSH_24         (6 << 2)
#define LIMCTL1_CUSH_30         (7 << 2)
#define LIMCTL1_LMAX_MASK       (7 << 5)
#define LIMCTL1_LMAX_0          (0 << 5)
#define LIMCTL1_LMAX_3          (1 << 5)
#define LIMCTL1_LMAX_6          (2 << 5)
#define LIMCTL1_LMAX_9          (3 << 5)
#define LIMCTL1_LMAX_12         (4 << 5)
#define LIMCTL1_LMAX_18         (5 << 5)
#define LIMCTL1_LMAX_24         (6 << 5)
#define LIMCTL1_LMAX_30         (7 << 5)

#define LIMCTL2                 0x22
#define LIMCTL2_LIMRRATE_MASK   (0x3f << 0)
#define LIMCTL2_LIMRRATE_SHIFT  0
#define LIMCTL2_LIMIT_ALL       (1 << 6)
#define LIMCTL2_LIMIT           (1 << 7)

#define LIMCTL3                 0x23
#define LIMCTL3_LIMARATE_MASK   (0x3f << 0)
#define LIMCTL3_LIMARATE_SHIFT  0

#define ALCCTL1                 0x24
#define ALCCTL1_ALCARATE_MASK   (0x3f << 0)
#define ALCCTL1_ALCARATE_SHIFT  0
#define ALCCTL1_ALCA            (1 << 6)
#define ALCCTL1_ALCB            (1 << 7)

#define ALCCTL2                 0x25
#define ALCCTL2_ALCRRATE_MASK   (0x3f << 0)
#define ALCCTL2_ALCRRATE_SHIFT  0

#define ALCCTL3                 0x26
#define ALCCTL3_ALCMIN_MASK     (7 << 2)
#define ALCCTL3_ALCMIN_0        (0 << 2)
#define ALCCTL3_ALCMIN_3        (1 << 2)
#define ALCCTL3_ALCMIN_6        (2 << 2)
#define ALCCTL3_ALCMIN_9        (3 << 2)
#define ALCCTL3_ALCMIN_12       (4 << 2)
#define ALCCTL3_ALCMIN_18       (5 << 2)
#define ALCCTL3_ALCMIN_24       (6 << 2)
#define ALCCTL3_ALCMIN_30       (7 << 2)
#define ALCCTL3_ALCMAX_MASK     (7 << 5)
#define ALCCTL3_ALCMAX_0        (0 << 5)
#define ALCCTL3_ALCMAX_3        (1 << 5)
#define ALCCTL3_ALCMAX_6        (2 << 5)
#define ALCCTL3_ALCMAX_9        (3 << 5)
#define ALCCTL3_ALCMAX_12       (4 << 5)
#define ALCCTL3_ALCMAX_18       (5 << 5)
#define ALCCTL3_ALCMAX_24       (6 << 5)
#define ALCCTL3_ALCMAX_30       (7 << 5)

#define NGCTL                   0x27
#define NGCTL_NGDELEAY_MASK     (3 << 0)
#define NGCTL_NGDELEAY_50       (0 << 0)
#define NGCTL_NGDELEAY_100      (1 << 0)
#define NGCTL_NGDELEAY_150      (2 << 0)
#define NGCTL_NGDELEAY_200      (3 << 0)
#define NGCTL_THRESH_MASK       (7 << 2)
#define NGCTL_THRESH_SHIFT      2       
#define NGCTL_NG_BOOST30        (1 << 5)
#define NGCTL_NG                (1 << 6)
#define NGCTL_NGALL             (1 << 7)

#define ALSZDIS                 0x28
#define ALSZDIS_LIMSRDIS        (1 << 3)
#define ALSZDIS_ALCAZCDIS       (1 << 4)
#define ALSZDIS_ALCASRDIS       (1 << 5)
#define ALSZDIS_ALCBZCDIS       (1 << 6)
#define ALSZDIS_ALCBSRDIS       (1 << 7)

#define STATUS                  0x29
#define STATUS_ADCAOVFL         (1 << 0)
#define STATUS_ADCBOVFL         (1 << 1)
#define STATUS_MIXAOVFL         (1 << 2)
#define STATUS_MIXBOVFL         (1 << 3)
#define STATUS_DSPAOVFL         (1 << 4)
#define STATUS_DSPBOVFL         (1 << 5)
#define STATUS_SPCLKERR         (1 << 6)
#define STATUS_HPDETECT         (1 << 7)

#define CPCTL                   0x2a
#define CPCTL_CHGFREQ_MASK      (0xf << 0)
#define CPCTL_CHGFREQ_SHIFT     0

#define HIDDEN2E                0x2e
#define HIDDEN2E_DEFAULT        0x30

#define HIDDEN32                0x32
#define HIDDEN32_DEFAULT        0x07

#define HIDDEN33                0x33
#define HIDDEN33_DEFAULT        0xff

#define HIDDEN34                0x34
#define HIDDEN34_DEFAULT        0xf8

#define HIDDEN35                0x35
#define HIDDEN35_DEFAULT        0xdc

#define HIDDEN36                0x36
#define HIDDEN36_DEFAULT        0xfc

#define HIDDEN37                0x37
#define HIDDEN37_DEFAULT        0xac

#define HIDDEN3A                0x3a
#define HIDDEN3A_DEFAULT        0xf8

#define HIDDEN3C                0x3c
#define HIDDEN3C_DEFAULT        0xd3

#define HIDDEN3D                0x3d
#define HIDDEN3D_DEFAULT        0x23

#define HIDDEN3E                0x3e
#define HIDDEN3E_DEFAULT        0x81

#define HIDDEN3F                0x3f
#define HIDDEN3F_DEFAULT        0x46


#endif /* __CS42L55_H__ */

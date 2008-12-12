/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Dave Chapman
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

#ifndef _WM8975_H
#define _WM8975_H

/* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -730
#define VOLUME_MAX  60

#define AUDIOHW_CAPS (BASS_CAP | TREBLE_CAP)

extern int tenthdb2master(int db);

extern void audiohw_set_master_vol(int vol_l, int vol_r);
extern void audiohw_set_lineout_vol(int vol_l, int vol_r);
extern void audiohw_set_frequency(int fsel);

/* Register addresses and bits */

#define LINVOL                  0x00
#define LINVOL_MASK             0x3f
#define LINVOL_LZCEN            (1 << 6)
#define LINVOL_LINMUTE          (1 << 7)
#define LINVOL_LIVU             (1 << 8)

#define RINVOL                  0x01
#define RINVOL_MASK             0x3f
#define RINVOL_RZCEN            (1 << 6)
#define RINVOL_RINMUTE          (1 << 7)
#define RINVOL_RIVU             (1 << 8)

#define LOUT1VOL                0x02
#define LOUT1VOL_MASK           0x7f
#define LOUT1VOL_LO1ZC          (1 << 7)
#define LOUT1VOL_LO1VU          (1 << 8)

#define ROUT1VOL                0x03
#define ROUT1VOL_MASK           0x7f
#define ROUT1VOL_RO1ZC          (1 << 7)
#define ROUT1VOL_RO1VU          (1 << 8)

#define DAPCTRL                 0x05  /* Digital audio path control */
#define DAPCTRL_ADCHPD          (1 << 0)
#define DAPCTRL_DEEMP_DISABLE   (0 << 1)
#define DAPCTRL_DEEMP_32KHz     (1 << 1)
#define DAPCTRL_DEEMP_44KHz     (2 << 1)
#define DAPCTRL_DEEMP_48KHz     (3 << 1)
#define DAPCTRL_DEEMP_MASK      (3 << 1)
#define DAPCTRL_DACMU           (1 << 3)
#define DAPCTRL_HPOR            (1 << 4)
#define DAPCTRL_ADCPOL_NORMAL   (0 << 5)
#define DAPCTRL_ADCPOL_LINVERT  (1 << 5)
#define DAPCTRL_ADCPOL_RINVERT  (2 << 5)
#define DAPCTRL_ADCPOL_LRINVERT (3 << 5)
#define DAPCTRL_ADCPOL_MASK     (3 << 5)
#define DAPCTRL_DACDIV2         (1 << 7)
#define DAPCTRL_ADCDIV2         (1 << 8)

#define AINTFCE                 0x07
#define AINTFCE_FORMAT_MSB_RJUST (0 << 0)
#define AINTFCE_FORMAT_MSB_LJUST (1 << 0)
#define AINTFCE_FORMAT_I2S      (2 << 0)
#define AINTFCE_FORMAT_DSP      (3 << 0)
#define AINTFCE_FORMAT_MASK     (3 << 0)
#define AINTFCE_IWL_16BIT       (0 << 2)
#define AINTFCE_IWL_20BIT       (1 << 2)
#define AINTFCE_IWL_24BIT       (2 << 2)
#define AINTFCE_IWL_32BIT       (3 << 2)
#define AINTFCE_IWL_MASK        (3 << 2)
#define AINTFCE_LRP_I2S_RLO     (0 << 4)
#define AINTFCE_LRP_I2S_RHI     (1 << 4)
#define AINTFCE_DSP_MODE_B      (0 << 4)
#define AINTFCE_DSP_MODE_A      (1 << 4)
#define AINTFCE_LRSWAP          (1 << 5)
#define AINTFCE_MS              (1 << 6)
#define AINTFCE_BCLKINV         (1 << 7)

#define SAMPCTRL                0x08
#define SAMPCTRL_USB            (1 << 0)
/* Bits 1-5:
 * Sample rate setting are device-specific. See datasheet
 * for proper settings for the device's clocking */
#define SAMPCTRL_SR_MASK        (0x1f << 1)
#define SAMPCTRL_CLKDIV2        (1 << 6)
#define SAMPCTRL_BCM_OFF        (0 << 7)
#define SAMPCTRL_BCM_MCLK_4     (1 << 7)
#define SAMPCTRL_BCM_MCLK_8     (2 << 7)
#define SAMPCTRL_BCM_MCLK_16    (3 << 7)

#define LDACVOL                 0x0a
#define LDACVOL_MASK            0xff
#define LDACVOL_LDVU            (1 << 8)

#define RDACVOL                 0x0b
#define RDACVOL_MASK            0xff
#define RDACVOL_RDVU            (1 << 8)

#define BASSCTRL                0x0c
#define BASSCTRL_MASK           0x0f
#define BASSCTRL_BC             (1 << 6)
#define BASSCTRL_BB             (1 << 7)

#define TREBCTRL                0x0d
#define TREBCTRL_MASK           0x0f
#define TREBCTRL_TC             (1 << 6)

#define RESET                   0x0f
#define RESET_RESET             0x0

/* not used atm */
#define ALC1                    0x11
#define ALC2                    0x12
#define ALC3                    0x13
#define NOISEGATE               0x14

#define LADCVOL                 0x15
#define LADCVOL_MASK            0xff
#define LADCVOL_LAVU            (1 << 8)

#define RADCVOL                 0x16
#define RADCVOL_MASK            0xff
#define RADCVOL_RAVU            (1 << 8)

#define ADDCTRL1                0x17
#define ADDCTRL1_TOEN           (1 << 0)
#define ADDCTRL1DACINV          (1 << 1)
#define ADDCTRL1_DATSEL_NORMAL  (0 << 2)
#define ADDCTRL1_DATSEL_LADC    (1 << 2)
#define ADDCTRL1_DATSEL_RADC    (2 << 2)
#define ADDCTRL1_DATSEL_SWAPPED (3 << 2)
#define ADDCTRL1_DMONOMIX_STEREO    (0 << 4)
#define ADDCTRL1_DMONOMIX_MONOLEFT  (1 << 4)
#define ADDCTRL1_DMONOMIX_MONORIGHT (2 << 4)
#define ADDCTRL1_DMONOMIX_MONO      (3 << 4)
#define ADDCTRL1_VSEL_HIGHBIAS  (0 << 6)
#define ADDCTRL1_VSEL_MEDBIAS   (1 << 6)
#define ADDCTRL1_VSEL_LOWBIAS   (3 << 6)
#define ADDCTRL1_TSDEN          (1 << 8)

#define ADDCTRL2                0x18
#define ADDCTRL2_DACOSR         (1 << 0)
#define ADDCTRL2_ADCOSR         (1 << 1)
#define ADDCTRL2_LRCM           (1 << 2)
#define ADDCTRL2_TRI            (1 << 3)
#define ADDCTRL2_ROUT2INV       (1 << 4)
#define ADDCTRL2_HPSWPOL        (1 << 5)
#define ADDCTRL2_HPSWEN         (1 << 6)
#define ADDCTRL2_OUT3SW_VREF    (0 << 7)
#define ADDCTRL2_OUT3SW_ROUT1   (1 << 7)
#define ADDCTRL2_OUT3SW_MONOOUT (2 << 7)
#define ADDCTRL2_OUT3SW_ROUTMIX (3 << 7)

#define PWRMGMT1                0x19
#define PWRMGMT1_DIGENB         (1 << 0)
#define PWRMGMT1_MICB           (1 << 1)
#define PWRMGMT1_ADCR           (1 << 2)
#define PWRMGMT1_ADCL           (1 << 3)
#define PWRMGMT1_AINR           (1 << 4)
#define PWRMGMT1_AINL           (1 << 5)
#define PWRMGMT1_VREF           (1 << 6)
#define PWRMGMT1_VMIDSEL_OFF    (0 << 7)
#define PWRMGMT1_VMIDSEL_50K    (1 << 7)
#define PWRMGMT1_VMIDSEL_500K   (2 << 7)
#define PWRMGMT1_VMIDSEL_5K     (3 << 7)
#define PWRMGMT1_VMIDSEL_MASK   (3 << 7)

#define PWRMGMT2                0x1a
#define PWRMGMT2_OUT3           (1 << 1)
#define PWRMGMT2_MONO           (1 << 2)
#define PWRMGMT2_ROUT2          (1 << 3)
#define PWRMGMT2_LOUT2          (1 << 4)
#define PWRMGMT2_ROUT1          (1 << 5)
#define PWRMGMT2_LOUT1          (1 << 6)
#define PWRMGMT2_DACR           (1 << 7)
#define PWRMGMT2_DACL           (1 << 8)

#define ADDCTRL3                0x1b
#define ADDCTRL3_HPFLREN        (1 << 5)
#define ADDCTRL3_VROI           (1 << 6)
#define ADDCTRL3_ADCLRM_IN      (0 << 7)
#define ADDCTRL3_ADCLRM_MCLK    (1 << 7)
#define ADDCTRL3_ADCLRM_MCLK_55 (2 << 7)
#define ADDCTRL3_ADCLRM_MCLK_6  (3 << 7)

#define ADCINMODE               0x1f
#define ADCINMODE_LDCM          (1 << 4)
#define ADCINMODE_RDCM          (1 << 5)
#define ADCINMODE_MONOMIX_STEREO  (0 << 6)
#define ADCINMODE_MONOMIX_LADC    (1 << 6)
#define ADCINMODE_MONOMIX_RADC    (2 << 6)
#define ADCINMODE_MONOMIX_DIGITAL (3 << 6)
#define ADCINMODE_DS            (1 << 8)

#define ADCLPATH                0x20
#define ADCLPATH_LMICBOOST_OFF  (0 << 4)
#define ADCLPATH_LMICBOOST_13dB (1 << 4)
#define ADCLPATH_LMICBOOST_20dB (2 << 4)
#define ADCLPATH_LMICBOOST_29dB (3 << 4)
#define ADCLPATH_LINSEL_LIN1    (0 << 6)
#define ADCLPATH_LINSEL_LIN2    (1 << 6)
#define ADCLPATH_LINSEL_LIN3    (2 << 6)
#define ADCLPATH_LINSEL_DIFF    (3 << 6)

#define ADCRPATH                0x21
#define ADCRPATH_RMICBOOST_OFF  (0 << 4)
#define ADCRPATH_RMICBOOST_13dB (1 << 4)
#define ADCRPATH_RMICBOOST_20dB (2 << 4)
#define ADCRPATH_RMICBOOST_29dB (3 << 4)
#define ADCRPATH_RINSEL_RIN1    (0 << 6)
#define ADCRPATH_RINSEL_RIN2    (1 << 6)
#define ADCRPATH_RINSEL_RIN3    (2 << 6)
#define ADCRPATH_RINSEL_DIFF    (3 << 6)

#define LOUTMIX1                0x22
#define LOUTMIX1_LMIXSEL_LIN1   (0 << 0)
#define LOUTMIX1_LMIXSEL_LIN2   (1 << 0)
#define LOUTMIX1_LMIXSEL_LIN3   (2 << 0)
#define LOUTMIX1_LMIXSEL_LADCIN (3 << 0)
#define LOUTMIX1_LMIXSEL_DIFF   (4 << 0)
#define LOUTMIX1_LI2LOVOL(x)    ((x & 7) << 4)
#define LOUTMIX1_LI2LOVOL_MASK  (7 << 4)
#define LOUTMIX1_LI2LO          (1 << 7)
#define LOUTMIX1_LD2LO          (1 << 8)

#define LOUTMIX2                0x23
#define LOUTMIX2_RI2LOVOL(x)    ((x & 7) << 4)
#define LOUTMIX2_RI2LOVOL_MASK  (7 << 4)
#define LOUTMIX2_RI2LO          (1 << 7)
#define LOUTMIX2_RD2LO          (1 << 8)

#define ROUTMIX1                0x24
#define ROUTMIX1_RMIXSEL_RIN1   (0 << 0)
#define ROUTMIX1_RMIXSEL_RIN2   (1 << 0)
#define ROUTMIX1_RMIXSEL_RIN3   (2 << 0)
#define ROUTMIX1_RMIXSEL_RADCIN (3 << 0)
#define ROUTMIX1_RMIXSEL_DIFF   (4 << 0)
#define ROUTMIX1_LI2ROVOL(x)    ((x & 7) << 4)
#define ROUTMIX1_LI2ROVOL_MASK  (7 << 4)
#define ROUTMIX1_LI2RO          (1 << 7)
#define ROUTMIX1_LD2RO          (1 << 8)

#define ROUTMIX2                0x25
#define ROUTMIX2_RI2ROVOL(x)    ((x & 7) << 4)
#define ROUTMIX2_RI2ROVOL_MASK  (7 << 4)
#define ROUTMIX2_RI2RO          (1 << 7)
#define ROUTMIX2_RD2RO          (1 << 8)

#define MOUTMIX1                0x26
#define MOUTMIX1_LI2MOVOL(x)    ((x & 7) << 4)
#define MOUTMIX1_LI2MOVOL_MASK  (7 << 4)
#define MOUTMIX1_LI2MO          (1 << 7)
#define MOUTMIX1_LD2MO          (1 << 8)

#define MOUTMIX2                0x27
#define MOUTMIX2_RI2MOVOL(x)    ((x & 7) << 4)
#define MOUTMIX2_RI2MOVOL_MASK  (7 << 4)
#define MOUTMIX2_RI2MO          (1 << 7)
#define MOUTMIX2_RD2MO          (1 << 8)

#define LOUT2VOL                0x28
#define LOUT2VOL_MASK           0x7f
#define LOUT2VOL_LO2ZC          (1 << 7)
#define LOUT2VOL_LO2VU          (1 << 8)

#define ROUT2VOL                0x29
#define ROUT2VOL_MASK           0x7f
#define ROUT2VOL_RO2ZC          (1 << 7)
#define ROUT2VOL_RO2VU          (1 << 8)

#define MOUTVOL                 0x2a
#define MOUTVOL_MASK            0x7f
#define MOUTVOL_MOZC            (1 << 7)


/* SAMPCTRL values for the supported samplerates: */
#define WM8975_8000HZ      0x4d
#define WM8975_12000HZ     0x61
#define WM8975_16000HZ     0x55
#define WM8975_22050HZ     0x77
#define WM8975_24000HZ     0x79
#define WM8975_32000HZ     0x59
#define WM8975_44100HZ     0x63
#define WM8975_48000HZ     0x41
#define WM8975_88200HZ     0x7f
#define WM8975_96000HZ     0x5d

#endif /* _WM8975_H */

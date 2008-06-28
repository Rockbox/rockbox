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

#ifndef _WM8731_H
#define _WM8731_H

/* volume/balance/treble/bass interdependency */
#define VOLUME_MIN -730
#define VOLUME_MAX  60

extern int tenthdb2master(int db);
extern int tenthdb2mixer(int db);

extern void audiohw_preinit(void);
extern void audiohw_set_master_vol(int vol_l, int vol_r);
extern void audiohw_set_nsorder(int order);
extern void audiohw_set_sample_rate(int sampling_control);

/* Register addresses and bits */
#define LINVOL                  0x00
#define LINVOL_MASK             0x1f
#define LINVOL_LRINBOTH         (1 << 8)
#define LINVOL_LINMUTE          (1 << 7)

#define RINVOL                  0x01
#define RINVOL_MASK             0x1f
#define RINVOL_RINMUTE          (1 << 7)
#define RINVOL_RLINBOTH         (1 << 8)

#define LOUTVOL                 0x02
#define LOUTVOL_LHPVOL_MASK     0x7f
#define LOUTVOL_LZCEN           (1 << 7)
#define LOUTVOL_LRHP_BOTH       (1 << 8)

#define ROUTVOL                 0x03
#define ROUTVOL_RHPVOL_MASK     0x7f
#define ROUTVOL_RZCEN           (1 << 7)
#define ROUTVOL_RLHP_BOTH       (1 << 8)

#define AAPCTRL                 0x04  /* Analog audio path control */
#define AAPCTRL_MIC_BOOST       (1 << 0)
#define AAPCTRL_MUTEMIC         (1 << 1)
#define AAPCTRL_INSEL           (1 << 2)
#define AAPCTRL_BYPASS          (1 << 3)
#define AAPCTRL_DACSEL          (1 << 4)
#define AAPCTRL_SIDETONE        (1 << 5)
#define AAPCTRL_SIDEATT_6dB     (3 << 6)
#define AAPCTRL_SIDEATT_9dB     (1 << 6)
#define AAPCTRL_SIDEATT_12dB    (2 << 6)
#define AAPCTRL_SIDEATT_15dB    (3 << 6)

#define DAPCTRL                 0x05  /* Digital audio path control */
#define DAPCTRL_ADCHPD          (1 << 0)
#define DAPCTRL_DEEMP_DISABLE   (0 << 2)
#define DAPCTRL_DEEMP_32KHz     (1 << 2)
#define DAPCTRL_DEEMP_44KHz     (2 << 2)
#define DAPCTRL_DEEMP_48KHz     (3 << 2)
#define DAPCTRL_DEEMP_MASK      (3 << 2)
#define DAPCTRL_DACMU           (1 << 3)
#define DAPCTRL_HPOR            (1 << 4)

#define PDCTRL                  0x06
#define PDCTRL_LINEINPD         (1 << 0)
#define PDCTRL_MICPD            (1 << 1)
#define PDCTRL_ADCPD            (1 << 2)
#define PDCTRL_DACPD            (1 << 3)
#define PDCTRL_OUTPD            (1 << 4)
#define PDCTRL_OSCPD            (1 << 5)
#define PDCTRL_CLKOUTPD         (1 << 6)
#define PDCTRL_POWEROFF         (1 << 7)

#define AINTFCE                     0x07
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
#define SAMPCTRL_BOSR_NOR_256fs (0 << 1)
#define SAMPCTRL_BOSR_NOR_384fs (1 << 1)
#define SAMPCTRL_BOSR_USB_250fs (0 << 1)
#define SAMPCTRL_BOSR_USB_272fs (1 << 1)
/* Bits 2-5:
 * Sample rate setting are device-specific. See WM8731(L) datasheet
 * for proper settings for the device's clocking */
#define SAMPCTRL_SR_MASK        (0xf << 2)
#define SAMPCTRL_CLKIDIV2       (1 << 6)
#define SAMPCTRL_CLKODIV2       (1 << 7)

#define ACTIVECTRL              0x09
#define ACTIVECTRL_ACTIVE       (1 << 0)

#define RESET                   0x0f
#define RESET_RESET             0x0

/* SAMPCTRL values for the supported samplerates (24MHz MCLK/USB): */
#define WM8731_USB24_8000HZ     0x4d
#define WM8731_USB24_32000HZ    0x59
#define WM8731_USB24_44100HZ    0x63
#define WM8731_USB24_48000HZ    0x41
#define WM8731_USB24_88200HZ    0x7f
#define WM8731_USB24_96000HZ    0x5d

#endif /* _WM8731_H */

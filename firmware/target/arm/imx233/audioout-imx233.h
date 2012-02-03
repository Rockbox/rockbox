/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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
#ifndef __audioout_imx233__
#define __audioout_imx233__

#include "config.h"
#include "cpu.h"
#include "system.h"

#define HW_AUDIOOUT_BASE       0x80048000 

#define HW_AUDIOOUT_CTRL        (*(volatile uint32_t *)(HW_AUDIOOUT_BASE + 0x0))
#define HW_AUDIOOUT_CTRL__RUN                   (1 << 0)
#define HW_AUDIOOUT_CTRL__FIFO_ERROR_IRQ_EN     (1 << 1)
#define HW_AUDIOOUT_CTRL__FIFO_OVERFLOW_IRQ     (1 << 2)
#define HW_AUDIOOUT_CTRL__FIFO_UNDERFLOW_IRQ    (1 << 3)
#define HW_AUDIOOUT_CTRL__WORD_LENGTH           (1 << 6)
#define HW_AUDIOOUT_CTRL__SS3D_EFFECT_BP        8
#define HW_AUDIOOUT_CTRL__SS3D_EFFECT_BM        (3 << 8)
#define HW_AUDIOOUT_CTRL__SS3D_EFFECT_OFF       (0 << 8)
#define HW_AUDIOOUT_CTRL__SS3D_EFFECT_3         (1 << 8)
#define HW_AUDIOOUT_CTRL__SS3D_EFFECT_4P5       (2 << 8)
#define HW_AUDIOOUT_CTRL__SS3D_EFFECT_6         (3 << 8)
#define HW_AUDIOOUT_CTRL__DMAWAIT_COUNT_BP      16
#define HW_AUDIOOUT_CTRL__DMAWAIT_COUNT_BM      (0x1f << 16)

#define HW_AUDIOOUT_DACSRR      (*(volatile uint32_t *)(HW_AUDIOOUT_BASE + 0x20))
#define HW_AUDIOOUT_DACSRR__SRC_FRAC_BP 0
#define HW_AUDIOOUT_DACSRR__SRC_FRAC_BM (0x1ff << 0)
#define HW_AUDIOOUT_DACSRR__SRC_INT_BP 16
#define HW_AUDIOOUT_DACSRR__SRC_INT_BM (0x1f << 16)
#define HW_AUDIOOUT_DACSRR__SRC_HOLD_BP 24
#define HW_AUDIOOUT_DACSRR__SRC_HOLD_BM (0x7 << 24)
#define HW_AUDIOOUT_DACSRR__BASEMULT_BP 28
#define HW_AUDIOOUT_DACSRR__BASEMULT_BM (0x7 << 28)
#define HW_AUDIOOUT_DACSRR__OSR (1 << 31)

#define HW_AUDIOOUT_DACVOLUME   (*(volatile uint32_t *)(HW_AUDIOOUT_BASE + 0x30))
#define HW_AUDIOOUT_DACVOLUME__VOLUME_RIGHT_BP  0
#define HW_AUDIOOUT_DACVOLUME__VOLUME_RIGHT_BM  0xff
#define HW_AUDIOOUT_DACVOLUME__MUTE_RIGHT       (1 << 8)
#define HW_AUDIOOUT_DACVOLUME__VOLUME_UPDATE_RIGHT  (1 << 12)
#define HW_AUDIOOUT_DACVOLUME__VOLUME_LEFT_BP   16
#define HW_AUDIOOUT_DACVOLUME__VOLUME_LEFT_BM   (0xff << 16)
#define HW_AUDIOOUT_DACVOLUME__MUTE_LEFT        (1 << 24)
#define HW_AUDIOOUT_DACVOLUME__EN_ZCD           (1 << 25)
#define HW_AUDIOOUT_DACVOLUME__VOLUME_UPDATE_LEFT   (1 << 28)

#define HW_AUDIOOUT_DACDEBUG    (*(volatile uint32_t *)(HW_AUDIOOUT_BASE + 0x40))
#define HW_AUDIOOUT_DACDEBUG__FIFO_STATUS   1


#define HW_AUDIOOUT_HPVOL       (*(volatile uint32_t *)(HW_AUDIOOUT_BASE + 0x50))
#define HW_AUDIOOUT_HPVOL__VOL_RIGHT_BP     0
#define HW_AUDIOOUT_HPVOL__VOL_RIGHT_BM     (0x7f << 0)
#define HW_AUDIOOUT_HPVOL__VOL_LEFT_BP      8
#define HW_AUDIOOUT_HPVOL__VOL_LEFT_BM      (0x7f << 8)
#define HW_AUDIOOUT_HPVOL__SELECT           (1 << 16)
#define HW_AUDIOOUT_HPVOL__MUTE             (1 << 24)
#define HW_AUDIOOUT_HPVOL__EN_MSTR_ZCD      (1 << 25)

#define HW_AUDIOOUT_PWRDN       (*(volatile uint32_t *)(HW_AUDIOOUT_BASE + 0x70))
#define HW_AUDIOOUT_PWRDN__HEADPHONE        (1 << 0)
#define HW_AUDIOOUT_PWRDN__CAPLESS          (1 << 4)
#define HW_AUDIOOUT_PWRDN__ADC              (1 << 8)
#define HW_AUDIOOUT_PWRDN__DAC              (1 << 12)
#define HW_AUDIOOUT_PWRDN__RIGHT_ADC        (1 << 16)
#define HW_AUDIOOUT_PWRDN__SPEAKER          (1 << 24)

#define HW_AUDIOOUT_REFCTRL     (*(volatile uint32_t *)(HW_AUDIOOUT_BASE + 0x80))
#define HW_AUDIOOUT_REFCTRL__LOW_PWR    (1 << 19)

#define HW_AUDIOOUT_ANACTRL     (*(volatile uint32_t *)(HW_AUDIOOUT_BASE + 0x90))
#define HW_AUDIOOUT_ANACTRL__HP_CLASSAB         (1 << 4)
#define HW_AUDIOOUT_ANACTRL__HP_HOLD_GND        (1 << 5)
#define HW_AUDIOOUT_ANACTRL__SHORTMODE_LR_BP    17
#define HW_AUDIOOUT_ANACTRL__SHORTMODE_LR_BM    (3 << 17)
#define HW_AUDIOOUT_ANACTRL__SHORTMODE_CM_BP    20
#define HW_AUDIOOUT_ANACTRL__SHORTMODE_CM_BM    (3 << 20)
#define HW_AUDIOOUT_ANACTRL__SHORT_LR_STS       (1 << 24)
#define HW_AUDIOOUT_ANACTRL__SHORT_CM_STS       (1 << 28)

#define HW_AUDIOOUT_ANACLKCTRL  (*(volatile uint32_t *)(HW_AUDIOOUT_BASE + 0xe0))
#define HW_AUDIOOUT_ANACLKCTRL__DACDIV_BP   0
#define HW_AUDIOOUT_ANACLKCTRL__DACDIV_BM   (7 << 0)
#define HW_AUDIOOUT_ANACLKCTRL__CLKGATE     (1 << 31)

#define HW_AUDIOOUT_DATA        (*(volatile uint32_t *)(HW_AUDIOOUT_BASE + 0xf0))

#define HW_AUDIOOUT_VERSION     (*(volatile uint32_t *)(HW_AUDIOOUT_BASE + 0x200))


void imx233_audioout_preinit(void);
void imx233_audioout_postinit(void);
void imx233_audioout_close(void);
/* volume in half dB */
void imx233_audioout_set_hp_vol(int vol_l, int vol_r);
/* frequency index, NOT the frequency itself */
void imx233_audioout_set_freq(int fsel);
/* select between DAC and Line1 */
void imx233_audioout_select_hp_input(bool line1);

#endif /* __audioout_imx233__ */

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
#ifndef __audioin_imx233__
#define __audioin_imx233__

#include "config.h"
#include "cpu.h"
#include "system.h"

#define HW_AUDIOIN_BASE         0x8004c000 

#define HW_AUDIOIN_CTRL         (*(volatile uint32_t *)(HW_AUDIOIN_BASE + 0x0))
#define HW_AUDIOIN_CTRL__RUN    (1 << 0)
#define HW_AUDIOIN_CTRL__FIFO_ERROR_IRQ_EN  (1 << 1)
#define HW_AUDIOIN_CTRL__FIFO_OVERFLOW_IRQ  (1 << 2)
#define HW_AUDIOIN_CTRL__FIFO_UNDERFLOW_IRQ (1 << 3)
#define HW_AUDIOIN_CTRL__LOOPBACK   (1 << 4)
#define HW_AUDIOIN_CTRL__WORD_LENGTH    (1 << 5)
#define HW_AUDIOIN_CTRL__HPF_ENABLE     (1 << 6)
#define HW_AUDIOIN_CTRL__OFFSET_ENABLE  (1 << 7)
#define HW_AUDIOIN_CTRL__INVERT_1BIT    (1 << 8)
#define HW_AUDIOIN_CTRL__EDGE_SYNC      (1 << 9)
#define HW_AUDIOIN_CTRL__LR_SWAP        (1 << 10)
#define HW_AUDIOIN_CTRL__DMAWAIT_COUNT_BP   16
#define HW_AUDIOIN_CTRL__DMAWAIT_COUNT_BM   (0x1f << 16)

#define HW_AUDIOIN_STAT         (*(volatile uint32_t *)(HW_AUDIOIN_BASE + 0x10))

#define HW_AUDIOIN_ADCSRR       (*(volatile uint32_t *)(HW_AUDIOIN_BASE + 0x20))
#define HW_AUDIOIN_ADCSRR__SRC_FRAC_BP  0
#define HW_AUDIOIN_ADCSRR__SRC_FRAC_BM  (0x1ff << 0)
#define HW_AUDIOIN_ADCSRR__SRC_INT_BP   16
#define HW_AUDIOIN_ADCSRR__SRC_INT_BM   (0x1f << 16)
#define HW_AUDIOIN_ADCSRR__SRC_HOLD_BP  24
#define HW_AUDIOIN_ADCSRR__SRC_HOLD_BM  (0x7 << 24)
#define HW_AUDIOIN_ADCSRR__BASEMULT_BP  28
#define HW_AUDIOIN_ADCSRR__BASEMULT_BM  (0x7 << 28)
#define HW_AUDIOIN_ADCSRR__OSR  (1 << 31)

/* MUTE_LEFT and MUTE_RIGHT are not documented but present */
#define HW_AUDIOIN_ADCVOLUME    (*(volatile uint32_t *)(HW_AUDIOIN_BASE + 0x30))
#define HW_AUDIOIN_ADCVOLUME__VOLUME_RIGHT_BP   0
#define HW_AUDIOIN_ADCVOLUME__VOLUME_RIGHT_BM   0xff
#define HW_AUDIOIN_ADCVOLUME__MUTE_RIGHT    (1 << 8)
#define HW_AUDIOIN_ADCVOLUME__VOLUME_UPDATE_RIGHT   (1 << 12)
#define HW_AUDIOIN_ADCVOLUME__VOLUME_LEFT_BP    16
#define HW_AUDIOIN_ADCVOLUME__VOLUME_LEFT_BM    (0xff << 16)
#define HW_AUDIOIN_ADCVOLUME__MUTE_LEFT (1 << 24)
#define HW_AUDIOIN_ADCVOLUME__EN_ZCD    (1 << 25)
#define HW_AUDIOIN_ADCVOLUME__VOLUME_UPDATE_LEFT    (1 << 28)

#define HW_AUDIOIN_ADCDEBUG     (*(volatile uint32_t *)(HW_AUDIOIN_BASE + 0x40))
#define HW_AUDIOIN_ADCDEBUG__FIFO_STATUS    1

#define HW_AUDIOIN_ADCVOL       (*(volatile uint32_t *)(HW_AUDIOIN_BASE + 0x50))
#define HW_AUDIOIN_ADCVOL__GAIN_RIGHT_BP    0 
#define HW_AUDIOIN_ADCVOL__GAIN_RIGHT_BM    (0xf << 0)
#define HW_AUDIOIN_ADCVOL__SELECT_RIGHT_BP  4
#define HW_AUDIOIN_ADCVOL__SELECT_RIGHT_BM  (0x3 << 4)
#define HW_AUDIOIN_ADCVOL__GAIN_LEFT_BP     8 
#define HW_AUDIOIN_ADCVOL__GAIN_LEFT_BM     (0xf << 8)
#define HW_AUDIOIN_ADCVOL__SELECT_LEFT_BP   12
#define HW_AUDIOIN_ADCVOL__SELECT_LEFT_BM   (0x3 << 12)
#define HW_AUDIOIN_ADCVOL__MUTE             (1 << 24)
#define HW_AUDIOIN_ADCVOL__EN_ADC_ZCD       (1 << 25)
#define HW_AUDIOIN_ADCVOL__VOLUME_UPDATE_PENDING    (1 << 28)

#define HW_AUDIOIN_MICLINE      (*(volatile uint32_t *)(HW_AUDIOIN_BASE + 0x60))
#define HW_AUDIOIN_MICLINE__MIC_GAIN_BP 0
#define HW_AUDIOIN_MICLINE__MIC_GAIN_BM 0x3
#define HW_AUDIOIN_MICLINE__MIC_CHOPCLK_BP  4
#define HW_AUDIOIN_MICLINE__MIC_CHOPCLK_BM  (0x3 << 4)
#define HW_AUDIOIN_MICLINE__MIC_BIAIS_BP    16
#define HW_AUDIOIN_MICLINE__MIC_BIAIS_BM    (0x3 << 16)
#define HW_AUDIOIN_MICLINE__MIC_RESISTOR_BP 20
#define HW_AUDIOIN_MICLINE__MIC_RESISTOR_BM (0x3 << 20)
#define HW_AUDIOIN_MICLINE__MIC_SELECT  (1 << 24)
#define HW_AUDIOIN_MICLINE__DIVIDE_LINE2    (1 << 28)
#define HW_AUDIOIN_MICLINE__DIVIDE_LINE1    (1 << 29)

#define HW_AUDIOIN_ANACLKCTRL   (*(volatile uint32_t *)(HW_AUDIOIN_BASE + 0x70))
#define HW_AUDIOIN_ANACLKCTRL__ADCDIV_BP    0
#define HW_AUDIOIN_ANACLKCTRL__ADCDIV_BM    (0x7 << 0)
#define HW_AUDIOIN_ANACLKCTRL__ADCCLK_SHIFT_BP  4
#define HW_AUDIOIN_ANACLKCTRL__ADCCLK_SHIFT_BM  (0x3 << 4)
#define HW_AUDIOIN_ANACLKCTRL__INVERT_ADCCLK    (1 << 8)
#define HW_AUDIOIN_ANACLKCTRL__SLOW_DITHER      (1 << 9)
#define HW_AUDIOIN_ANACLKCTRL__DITHER_OFF       (1 << 10)
#define HW_AUDIOIN_ANACLKCTRL__CLKGATE          (1 << 31)

#define HW_AUDIOIN_DATA         (*(volatile uint32_t *)(HW_AUDIOIN_BASE + 0x80))

void imx233_audioin_preinit(void);
void imx233_audioin_postinit(void);
void imx233_audioin_close(void);

#endif /* __audioin_imx233__ */

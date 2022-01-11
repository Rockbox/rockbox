/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021-2022 Aidan MacDonald
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

#ifndef __X1000_CODEC_H__
#define __X1000_CODEC_H__

#include "config.h"
#include <stdbool.h>

/* Note: the internal X1000 codec supports playback and record, but devices
 * can employ an external codec for one and the internal codec for the other.
 * The caveat, in this case, is that only one codec can be used at a time
 * because the HW cannot mux playback/record independently.
 *
 * At present only recording is implemented, since all X1000 ports use an
 * external DAC for playback.
 */

#ifdef HAVE_X1000_ICODEC_PLAY
# error "X1000 icodec playback not implemented"
#endif

#define X1000_ICODEC_ADC_GAIN_MIN  0
#define X1000_ICODEC_ADC_GAIN_MAX  43
#define X1000_ICODEC_ADC_GAIN_STEP 1

#define X1000_ICODEC_MIC_GAIN_MIN  0
#define X1000_ICODEC_MIC_GAIN_MAX  20
#define X1000_ICODEC_MIC_GAIN_STEP 4

#ifdef HAVE_X1000_ICODEC_REC
AUDIOHW_SETTING(MIC_GAIN, "dB", 0, 1, 0, 63, 12)
#endif

#define JZCODEC_INDIRECT_CREG(r)  ((r) & 0xff)
#define JZCODEC_INDIRECT_INDEX(r) (((r) >> 8) & 0x7)
#define JZCODEC_INDIRECT_BIT      0x800

#define JZCODEC_INDIRECT(c, i) (JZCODEC_INDIRECT_BIT | ((i) << 8) | (c))

/* Codec registers from Ingenic's kernel sources. The datasheet is badly
 * screwed up and the addresses listed cannot be trusted. */
enum {
    JZCODEC_SR = 0,
    JZCODEC_SR2,
    JZCODEC_SIGR,
    JZCODEC_SIGR2,
    JZCODEC_SIGR3,
    JZCODEC_SIGR5,
    JZCODEC_SIGR7,
    JZCODEC_MR,
    JZCODEC_AICR_DAC,
    JZCODEC_AICR_ADC,
    JZCODEC_CR_DMIC,
    JZCODEC_CR_MIC1,
    JZCODEC_CR_MIC2,
    JZCODEC_CR_DAC,
    JZCODEC_CR_DAC2,
    JZCODEC_CR_ADC,
    JZCODEC_CR_MIX,
    JZCODEC_DR_MIX,
    JZCODEC_CR_VIC,
    JZCODEC_CR_CK,
    JZCODEC_FCR_DAC,
    JZCODEC_SFCCR_DAC,
    JZCODEC_SFFCR_DAC,
    JZCODEC_FCR_ADC,
    JZCODEC_CR_TIMER_MSB,
    JZCODEC_CR_TIMER_LSB,
    JZCODEC_ICR,
    JZCODEC_IMR,
    JZCODEC_IFR,
    JZCODEC_IMR2,
    JZCODEC_IFR2,
    JZCODEC_GCR_DACL,
    JZCODEC_GCR_DACR,
    JZCODEC_GCR_DACL2,
    JZCODEC_GCR_DACR2,
    JZCODEC_GCR_MIC1,
    JZCODEC_GCR_MIC2,
    JZCODEC_GCR_ADCL,
    JZCODEC_GCR_ADCR,
    JZCODEC_GCR_MIXDACL,
    JZCODEC_GCR_MIXDACR,
    JZCODEC_GCR_MIXADCL,
    JZCODEC_GCR_MIXADCR,
    JZCODEC_CR_DAC_AGC,
    JZCODEC_DR_DAC_AGC,
    JZCODEC_CR_DAC2_AGC,
    JZCODEC_DR_DAC2_AGC,
    JZCODEC_CR_ADC_AGC,
    JZCODEC_DR_ADC_AGC,
    JZCODEC_SR_ADC_AGCDGL,
    JZCODEC_SR_ADC_AGCDGR,
    JZCODEC_SR_ADC_AGCAGL,
    JZCODEC_SR_ADC_AGCAGR,
    JZCODEC_CR_TR,
    JZCODEC_DR_TR,
    JZCODEC_SR_TR1,
    JZCODEC_SR_TR2,
    JZCODEC_SR_TR_SRCDAC,

    JZCODEC_MIX0 = JZCODEC_INDIRECT(JZCODEC_CR_MIX, 0),
    JZCODEC_MIX1 = JZCODEC_INDIRECT(JZCODEC_CR_MIX, 1),
    JZCODEC_MIX2 = JZCODEC_INDIRECT(JZCODEC_CR_MIX, 2),
    JZCODEC_MIX3 = JZCODEC_INDIRECT(JZCODEC_CR_MIX, 3),
    JZCODEC_MIX4 = JZCODEC_INDIRECT(JZCODEC_CR_MIX, 4),

    JZCODEC_DAC_AGC0 = JZCODEC_INDIRECT(JZCODEC_CR_DAC_AGC, 0),
    JZCODEC_DAC_AGC1 = JZCODEC_INDIRECT(JZCODEC_CR_DAC_AGC, 1),
    JZCODEC_DAC_AGC2 = JZCODEC_INDIRECT(JZCODEC_CR_DAC_AGC, 2),
    JZCODEC_DAC_AGC3 = JZCODEC_INDIRECT(JZCODEC_CR_DAC_AGC, 3),

    JZCODEC_DAC2_AGC0 = JZCODEC_INDIRECT(JZCODEC_CR_DAC2_AGC, 0),
    JZCODEC_DAC2_AGC1 = JZCODEC_INDIRECT(JZCODEC_CR_DAC2_AGC, 1),
    JZCODEC_DAC2_AGC2 = JZCODEC_INDIRECT(JZCODEC_CR_DAC2_AGC, 2),
    JZCODEC_DAC2_AGC3 = JZCODEC_INDIRECT(JZCODEC_CR_DAC2_AGC, 3),

    JZCODEC_ADC_AGC0 = JZCODEC_INDIRECT(JZCODEC_CR_ADC_AGC, 0),
    JZCODEC_ADC_AGC1 = JZCODEC_INDIRECT(JZCODEC_CR_ADC_AGC, 1),
    JZCODEC_ADC_AGC2 = JZCODEC_INDIRECT(JZCODEC_CR_ADC_AGC, 2),
    JZCODEC_ADC_AGC3 = JZCODEC_INDIRECT(JZCODEC_CR_ADC_AGC, 3),
    JZCODEC_ADC_AGC4 = JZCODEC_INDIRECT(JZCODEC_CR_ADC_AGC, 4),
};

/* for use with x1000_icodec_mic1_configure() */
enum {
    JZCODEC_MIC1_SINGLE_ENDED = (0 << 6),
    JZCODEC_MIC1_DIFFERENTIAL = (1 << 6),

    JZCODEC_MIC1_BIAS_2_08V = (0 << 3),
    JZCODEC_MIC1_BIAS_1_66V = (1 << 3),

    JZCODEC_MIC1_CONFIGURE_MASK = (1 << 6) | (1 << 3),
};

/* for use with x1000_icodec_adc_mic_sel() */
enum {
    JZCODEC_MIC_SEL_ANALOG,
    JZCODEC_MIC_SEL_DIGITAL,
};

extern void x1000_icodec_open(void);
extern void x1000_icodec_close(void);

extern void x1000_icodec_dac_frequency(int fsel);

extern void x1000_icodec_adc_enable(bool en);
extern void x1000_icodec_adc_mute(bool muted);
extern void x1000_icodec_adc_mic_sel(int sel);
extern void x1000_icodec_adc_frequency(int fsel);
extern void x1000_icodec_adc_highpass_filter(bool en);
extern void x1000_icodec_adc_gain(int gain_dB);

extern void x1000_icodec_mic1_enable(bool en);
extern void x1000_icodec_mic1_bias_enable(bool en);
extern void x1000_icodec_mic1_configure(int settings);
extern void x1000_icodec_mic1_gain(int gain_dB);

extern void x1000_icodec_mixer_enable(bool en);

extern int  x1000_icodec_read(int reg);
extern void x1000_icodec_write(int reg, int value);
extern void x1000_icodec_update(int reg, int mask, int value);

#endif /* __X1000_CODEC_H__ */

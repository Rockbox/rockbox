/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifdef HAVE_X1000_ICODEC_REC
// FIXME: range
AUDIOHW_SETTING(MIC_GAIN, "dB", 0, 1, -999, 999, 999)
#endif

/* Codec registers */
#define JZCODEC_SR             0x00
#define JZCODEC_SR2            0x01
#define JZCODEC_SIGR           0x02
#define JZCODEC_SIGR2          0x03
#define JZCODEC_SIGR3          0x04
#define JZCODEC_SIGR5          0x05
#define JZCODEC_SIGR7          0x06
#define JZCODEC_MR             0x07
#define JZCODEC_AICR_DAC       0x08
#define JZCODEC_AICR_ADC       0x09
#define JZCODEC_CR_DMIC        0x0a
#define JZCODEC_CR_MIC1        0x0b
#define JZCODEC_CR_MIC2        0x0c
#define JZCODEC_CR_DAC         0x0d
#define JZCODEC_CR_DAC2        0x0e
#define JZCODEC_CR_ADC         0x0f
#define JZCODEC_CR_MIX         0x10
#define JZCODEC_DR_MIX         0x11 /* 0x1a? */
#define JZCODEC_CR_VIC         0x12
#define JZCODEC_CR_CK          0x13
#define JZCODEC_FCR_DAC        0x14
#define JZCODEC_SFCCR_DAC      0x15
#define JZCODEC_SFFCR_DAC      0x16
#define JZCODEC_FCR_ADC        0x17
#define JZCODEC_CR_TIMER_MSB   0x18
#define JZCODEC_CR_TIMER_LSB   0x19
#define JZCODEC_ICR            0x1a
#define JZCODEC_IMR            0x1b
#define JZCODEC_IFR            0x1c
#define JZCODEC_IMR2           0x1d
#define JZCODEC_IFR2           0x1e
#define JZCODEC_GCR_DACL       0x1f
#define JZCODEC_GCR_DACR       0x20
#define JZCODEC_GCR_DAC2L      0x21
#define JZCODEC_GCR_DAC2R      0x22
#define JZCODEC_GCR_MIC1       0x23
#define JZCODEC_GCR_MIC2       0x24
#define JZCODEC_GCR_ADCL       0x25
#define JZCODEC_GCR_ADCR       0x26
#define JZCODEC_GCR_MIXDACL    0x27
#define JZCODEC_GCR_MIXDACR    0x28
#define JZCODEC_GCR_MIXADCL    0x29
#define JZCODEC_GCR_MIXADCR    0x2a
#define JZCODEC_CR_DAC_AGC     0x2b
#define JZCODEC_DR_DAC_AGC     0x2c
#define JZCODEC_CR_DAC2_AGC    0x2d
#define JZCODEC_DR_DAC2_AGC    0x2e
#define JZCODEC_CR_ADC_AGC     0x2f
#define JZCODEC_DR_ADC_AGC     0x30
#define JZCODEC_SR_ADC_AGCDGL  0x31
#define JZCODEC_SR_ADC_AGCDGR  0x32
#define JZCODEC_SR_ADC_AGCAGL  0x33
#define JZCODEC_SR_ADC_AGCAGR  0x34
#define JZCODEC_CR_TR          0x35
#define JZCODEC_DR_TR          0x36
#define JZCODEC_SR_TR1         0x37
#define JZCODEC_SR_TR2         0x38
#define JZCODEC_SR_SRCDAC      0x39
#define JZCODEC_NUM_REGS       0x3a

extern void x1000_icodec_open(void);
extern void x1000_icodec_close(void);

extern int  x1000_icodec_read(int reg);
extern void x1000_icodec_write(int reg, int value);

#endif /* __X1000_CODEC_H__ */

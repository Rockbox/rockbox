/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for internal Rockchip rk27xx audio codec
 * (shCODlp-100.01-HD IP core from Dolphin)
 *
 * Copyright (c) 2011 Marcin Bukat
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
#ifndef _RK27XX_CODEC_H_
#define _RK27XX_CODEC_H_

#define AUDIOHW_CAPS    (BASS_CAP | TREBLE_CAP | LIN_GAIN_CAP | MIC_GAIN_CAP)

AUDIOHW_SETTING(VOLUME,     "dB", 0,  1,   -34,    4, -25)
#ifdef HAVE_RECORDING /* disabled for now */
AUDIOHW_SETTING(LEFT_GAIN,  "dB", 2, 75, -1725, 3000,   0)
AUDIOHW_SETTING(RIGHT_GAIN, "dB", 2, 75, -1725, 3000,   0)
AUDIOHW_SETTING(MIC_GAIN,   "dB", 0,  1,     0,   20,  20)
#endif /* HAVE_RECORDING */

#define CODEC_I2C_ADDR 0x4e

/* registers */
#define AICR  0x00     /* Audio Interface Control    */
#define DAC_SERIAL     (1<<3)
#define ADC_SERIAL     (1<<2)
#define DAC_I2S        (1<<1)
#define ADC_I2S        (1<<0)

#define CR1   0x02     /* Control Register 1         */
#define SB_MICBIAS     (1<<7)
#define CR1_MONO       (1<<6)
#define DAC_MUTE       (1<<5)
#define HP_DIS         (1<<4)
#define DACSEL         (1<<3)
#define BYPASS1        (1<<2)
#define BYPASS2        (1<<1)
#define SIDETONE       (1<<0)

#define CR2   0x04     /* Control Register 2         */
#define DAC_DEEMP      (1<<7)
#define ADC_HPF        (1<<2)
#define INSEL_MIX      (3<<0)
#define INSEL_MIC      (2<<0)
#define INSEL_LINE2    (1<<0)
#define INSEL_LINE1    (0<<0)

#define CCR1  0x06     /* Control Clock Register 1   */
#define CRYSTAL_16M    (1<<0)
#define CRYSTAL_12M    (0<<0)

#define CCR2  0x08     /* Control Clock Register 2   */
#define FREQ8000       0x0a
#define FREQ9600       0x09
#define FREQ11025      0x08
#define FREQ12000      0x07
#define FREQ16000      0x06
#define FREQ22050      0x05
#define FREQ24000      0x04
#define FREQ32000      0x03
#define FREQ44100      0x02
#define FREQ48000      0x01
#define FREQ96000      0x00

#define PMR1  0x0a     /* Power Mode Register 1      */
#define SB_DAC         (1<<7)
#define SB_OUT         (1<<6)
#define SB_MIX         (1<<5)
#define SB_ADC         (1<<4)
#define SB_IN1         (1<<3)
#define SB_IN2         (1<<2)
#define SB_MIC         (1<<1)
#define SB_IND         (1<<0)

#define PMR2  0x0c     /* Power Mode Register 1      */
#define LRGI           (1<<7)
#define RLGI           (1<<6)
#define LRGOD          (1<<5)
#define RLGOD          (1<<4)
#define GIM            (1<<3)
#define SB_MC          (1<<2)
#define SB             (1<<1)
#define SB_SLEEP       (1<<0)

#define CRR   0x0e     /* Control Ramp Register      */
#define RATIO_8        (3<<5)
#define RATIO_4        (2<<5)
#define RATIO_2        (1<<5)
#define RATIO_1        (0<<5)
#define KFAST_32       (5<<2)
#define KFAST_16       (4<<2)
#define KFAST_8        (3<<2)
#define KFAST_4        (2<<2)
#define KFAST_2        (1<<2)
#define KFAST_1        (0<<2)
#define THRESHOLD_128  (3<<0)
#define THRESHOLD_64   (2<<0)
#define THRESHOLD_32   (1<<0)
#define THRESHOLD_0    (0<<0)

#define ICR   0x10     /* Interrupt Control Register */
#define IRQ_LOW_PULSE  (3<<6)
#define IRQ_HIGH_PULSE (2<<6)
#define IRQ_LOW        (1<<6)
#define IRQ_HIGH       (0<<6)
#define JACK_MASK      (1<<5)
#define CCMC_MASK      (1<<4)
#define RUD_MASK       (1<<3)
#define RDD_MASK       (1<<2)
#define GUD_MASK       (1<<1)
#define GDD_MASK       (1<<0)

#define IFR   0x12     /* Interrupt Flag Register    */
#define JACK           (1<<6)
#define JACK_EVENT     (1<<5)
#define CCMC           (1<<4)
#define RAMP_UP_DONE   (1<<3)
#define RAMP_DOWN_DONE (1<<2)
#define GAIN_UP_DONE   (1<<1)
#define GAIN_DOWN_DONE (1<<0)

#define CGR1  0x14     /* Control Gain Register 1 (DAC mixing)      */

#define CGR2  0x16     /* Control Gain Register 2 (LINE1 mixing)    */
#define LRGOB1         (1<<7)
#define RLGOB1         (1<<6)

#define CGR3  0x18     /* Control Gain Register 3 (LINE1 mixing)    */

#define CGR4  0x1a     /* Control Gain Register 4 (LINE2 mixing)    */
#define LRGOB2         (1<<7)
#define RLGOB2         (1<<6)

#define CGR5  0x1c     /* Control Gain Register 5 (LINE2 mixing)    */

#define CGR6  0x1e     /* Control Gain Register 6 (MIC mixing)      */
#define LRGOS          (1<<7)
#define RLGOS          (1<<6)

#define CGR7  0x20     /* Control Gain Register 7 (MIC mixing)      */

#define CGR8  0x22     /* Control Gain Register 8 (OUT STAGE gain)  */
#define LRGO           (1<<7)
#define RLGO           (1<<6)

#define CGR9  0x24     /* Control Gain Register 9 (OUT STAGE gain)  */

#define CGR10 0x26     /* Control Gain Register 10 (ADC input gain) */

#define TR1   0x28     /* undocumented */
#define NOSC           (1<<1)

#define TR2   0x2a     /* undocumented */

#endif /* _RK27XX_CODEC_H_ */

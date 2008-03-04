/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Daniel Ankers
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _AS3514_H
#define _AS3514_H

#include <stdbool.h>

extern int tenthdb2master(int db);

extern void audiohw_set_master_vol(int vol_l, int vol_r);
extern void audiohw_set_lineout_vol(int vol_l, int vol_r);
extern void audiohw_set_sample_rate(int sampling_control);

/* Register Descriptions */
#define LINE_OUT_R 0x00
#define LINE_OUT_L 0x01
#define HPH_OUT_R  0x02
#define HPH_OUT_L  0x03
#define LSP_OUT_R  0x04
#define LSP_OUT_L  0x05
#define MIC1_R     0x06
#define MIC1_L     0x07
#define MIC2_R     0x08
#define MIC2_L     0x09
#define LINE_IN1_R 0x0a
#define LINE_IN1_L 0x0b
#define LINE_IN2_R 0x0c
#define LINE_IN2_L 0x0d
#define DAC_R      0x0e
#define DAC_L      0x0f
#define ADC_R      0x10
#define ADC_L      0x11
#define AUDIOSET1  0x14
#define AUDIOSET2  0x15
#define AUDIOSET3  0x16
#define PLLMODE    0x1d

#define SYSTEM     0x20
#define CHRGR      0x22
#define DCDC15     0x23
#define SUPERVISOR 0x24

#define IRQ_ENRD0  0x25
#define IRQ_ENRD1  0x26
#define IRQ_ENRD2  0x27

#define RTC_0      0x2a
#define RTC_1      0x2b
#define RTC_2      0x2c
#define RTC_3      0x2d
#define ADC_0      0x2e
#define ADC_1      0x2f

/* Headphone volume goes from -73.5 ... +6dB */
#define VOLUME_MIN -735
#define VOLUME_MAX   60

#if defined(SANSA_E200) || defined(SANSA_C200)
#define AS3514_I2C_ADDR 0x46
#endif

#endif /* _AS3514_H */

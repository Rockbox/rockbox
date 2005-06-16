/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _UDA1380_H
#define _UDA1380_H

extern int uda1380_init(void);
extern void uda1380_enable_output(bool enable);
extern int uda1380_setvol(int vol);
extern void uda1380_set_balance(int bal);
extern void uda1380_set_bass(int value);
extern void uda1380_set_treble(int value);
extern int uda1380_mute(int mute);
extern void uda1380_close(void);

extern void uda1380_enable_recording(bool source_mic);
extern void uda1380_disable_recording(void);
extern void uda1380_set_recvol(int mic_gain, int linein_gain, int adc_volume);
extern void uda1380_set_monitor(int enable);

#define UDA1380_ADDR        0x30

/* REG_0: Misc settings */
#define REG_0               0x00

#define EN_ADC              (1 << 11)    /* Enable ADC                     */
#define EN_DEC              (1 << 10)    /* Enable Decimator               */
#define EN_DAC              (1 << 9)     /* Enable DAC                     */
#define EN_INT              (1 << 8)     /* Enable Interpolator            */
#define ADC_CLK             (1 << 5)     /* ADC_CLK: WSPLL (1) SYSCLK (0)  */
#define DAC_CLK             (1 << 4)     /* DAC_CLK: WSPLL (1) SYSCLK (0)  */

/* SYSCLK freqency select */
#define SYSCLK_256FS        (0 << 2)
#define SYSCLK_384FS        (1 << 2)
#define SYSCLK_512FS        (2 << 2)
#define SYSCLK_768FS        (3 << 2)

/* WSPLL Input frequency range (kHz) */
#define WSPLL_625_125       (0 << 0)     /* 6.25 - 12.5      */
#define WSPLL_125_25        (1 << 0)     /* 12.5 - 25        */
#define WSPLL_25_50         (2 << 0)     /* 25   - 50        */
#define WSPLL_50_100        (3 << 0)     /* 50   - 100       */


/* REG_I2S: I2S settings */
#define REG_I2S     		0x01
#define I2S_IFMT_IIS        (0 << 8)
#define I2S_IFMT_LSB16      (1 << 8)
#define I2S_IFMT_LSB18      (2 << 8)
#define I2S_IFMT_LSB20      (3 << 8)
#define I2S_IFMT_MSB        (5 << 8)
#define I2S_OFMT_IIS        (0 << 0)
#define I2S_OFMT_LSB16      (1 << 0)
#define I2S_OFMT_LSB18      (2 << 0)
#define I2S_OFMT_LSB20      (3 << 0)
#define I2S_OFMT_LSB24      (4 << 0)
#define I2S_OFMT_MSB        (5 << 0)
#define I2S_MODE_MASTER     (1 << 4)

/* REG_PWR: Power control */
#define REG_PWR             0x02
#define PON_PLL             (1 << 15)   /* Power-on WSPLL                     */
#define PON_HP              (1 << 13)   /* Power-on Headphone driver          */
#define PON_DAC             (1 << 10)   /* Power-on DAC                       */
#define PON_BIAS            (1 << 8)    /* Power-on BIAS for ADC, AVC, FSDAC  */ 
#define EN_AVC              (1 << 7)    /* Enable analog mixer                */
#define PON_AVC             (1 << 6)    /* Power-on analog mixer              */
#define PON_LNA             (1 << 4)    /* Power-on LNA & SDC                 */
#define PON_PGAL            (1 << 3)    /* Power-on PGA left                  */
#define PON_ADCL            (1 << 2)    /* Power-on ADC left                  */
#define PON_PGAR            (1 << 1)    /* Power-on PGA right                 */
#define PON_ADCR            (1 << 0)    /* Power-on ADC right                 */


/* REG_AMIX: Analog mixer */
#define REG_AMIX            0x03
#define AMIX_LEFT(x)        (((x) & 0x3f) << 8)
#define AMIX_RIGHT(x)       (((x) & 0x3f) << 0)

/* REG_HP: Headphone amp */
#define REG_HP              0x04

/* REG_MV: Master Volume control */
#define REG_MASTER_VOL      0x10

#define MASTER_VOL_RIGHT(x) (((x) & 0xff) << 8)
#define MASTER_VOL_LEFT(x)  (((x) & 0xff) << 0)

/* REG_MIX: Mixer volume control */
/* Channel 1 is from digital data from I2S */
/* Channel 2 is from decimation filter */

#define REG_MIX_VOL         0x11
#define MIX_VOL_CH_1(x)     (((x) & 0xff) << 0)
#define MIX_VOL_CH_2(x)     (((x) & 0xff) << 8)

/* REG_EQ: Bass boost and tremble */
#define REG_EQ              0x12
#define EQ_MODE_FLAG        (0 << 14)
#define EQ_MODE_MIN         (1 << 14)
#define EQ_MODE_MAX         (3 << 14)
#define BASSL(x)            (((x) & 0xF) << 8)
#define BASSR(x)            (((x) & 0xF) << 0)
#define TREBLEL(x)          (((x) & 0x3) << 12)
#define TREBLER(x)          (((x) & 0x3) << 4)
#define BASS_MASK           0x0F0F
#define TREBLE_MASK         0x3030

/* REG_MUTE: Master Mute, silence detector and oversampling */
#define REG_MUTE            0x13
#define MUTE_MASTER         (1 << 14)      /* Master Mute (soft)       */
#define MIX_MODE(x)         ((x) << 12)    /* Mixer mode: See table 48 */ 
#define MUTE_CH2            (1 << 11)      /* Channel 2 mute           */
#define MUTE_CH1            (1 << 3)       /* Channel 1 mute           */


/* REG_MIX_CTL: Mixer, silence detector and oversampling settings */
#define REG_MIX_CTL         0x14
#define MIX_CTL_MIX_POS     (1 << 13)
#define MIX_CTL_MIX         (1 << 12)

/* REG_DEC_VOL: Decimator (ADC) volume control */
#define REG_DEC_VOL         0x20
#define DEC_VOLL(x)         (((x) & 0xff) << 8)
#define DEC_VOLR(x)         (((x) & 0xff) << 0)

/* REG_PGA: PGA settings and mute */
#define REG_PGA             0x21
#define MUTE_ADC            (1 << 15)               /* Mute ADC */
#define PGA_GAINR(x)        (((x) & 0xF) << 8)
#define PGA_GAINL(x)        (((x) & 0xF) << 0)
#define PGA_GAIN_MASK       0x0F0F

/* REG_ADC: */
#define REG_ADC             0x22
#define SEL_LNA             (1 << 3)
#define SEL_MIC             (1 << 2)
#define SKIP_DCFIL          (1 << 1)
#define EN_DCFIL            (1 << 0)
#define VGA_GAIN(x)         (((x) & 0xF) << 8)
#define VGA_GAIN_MASK       0x0F00

/* REG_AGC: Attack / Gain */
#define REG_AGC             0x23

#endif /* _UDA_1380_H */

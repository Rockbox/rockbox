/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Andy Young
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "lcd.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "power.h"
#include "debug.h"
#include "system.h"
#include "sprintf.h"
#include "button.h"
#include "string.h"
#include "file.h"
#include "buffer.h"

#include "i2c-h100.h"
#include "uda1380.h"

/* ------------------------------------------------- */
/* Local functions and variables */
/* ------------------------------------------------- */

int uda1380_write_reg(unsigned char reg, unsigned short value);
unsigned short uda1380_regs[0x30];
short uda1380_balance;
short uda1380_volume;

/* Definition of a playback configuration to start with */

#define NUM_DEFAULT_REGS 13
unsigned short uda1380_defaults[2*NUM_DEFAULT_REGS] =
{
   REG_0,          EN_DAC | EN_INT | EN_DEC | SYSCLK_256FS | WSPLL_25_50,
   REG_I2S,        I2S_IFMT_IIS,
   REG_PWR,        PON_PLL | PON_DAC | PON_BIAS,                   /* PON_HP is enabled later */
   REG_AMIX,       AMIX_RIGHT(0x3f) | AMIX_LEFT(0x3f),             /* 00=max, 3f=mute */
   REG_MASTER_VOL, MASTER_VOL_LEFT(0x20) | MASTER_VOL_RIGHT(0x20), /* 00=max, ff=mute */
   REG_MIX_VOL,    MIX_VOL_CH_1(0) | MIX_VOL_CH_2(0xff),           /* 00=max, ff=mute */
   REG_EQ,         EQ_MODE_MAX,                                    /* Bass and tremble = 0 dB */
   REG_MUTE,       MUTE_MASTER,                                    /* Mute everything to start with */ 
   REG_MIX_CTL,    0,
   REG_DEC_VOL,    0,
   REG_PGA,        MUTE_ADC,
   REG_ADC,        SKIP_DCFIL,
   REG_AGC,        0
};

  

/* Returns 0 if register was written or -1 if write failed */
int uda1380_write_reg(unsigned char reg, unsigned short value)
{
    unsigned char data[4];

    data[0] = UDA1380_ADDR;
    data[1] = reg;
    data[2] = value >> 8;
    data[3] = value & 0xff;

    if (i2c_write(1, data, 4) != 4)
    {
        DEBUGF("uda1380 error reg=0x%x", reg);
        return -1;
    } 

    uda1380_regs[reg] = value;

    return 0;
}

/**
 * Sets left and right master volume  (0(max) to 252(muted))
 */
int uda1380_setvol(int vol_l, int vol_r)
{
    return uda1380_write_reg(REG_MASTER_VOL,
                             MASTER_VOL_LEFT(vol_l) | MASTER_VOL_RIGHT(vol_r));
}

/**
 * Sets the bass value (0-15)
 */
void uda1380_set_bass(int value)
{
    uda1380_write_reg(REG_EQ, (uda1380_regs[REG_EQ] & ~BASS_MASK) | BASSL(value) | BASSR(value));
}

/**
 * Sets the treble value (0-3)
 */
void uda1380_set_treble(int value)
{
    uda1380_write_reg(REG_EQ, (uda1380_regs[REG_EQ] & ~TREBLE_MASK) | TREBLEL(value) | TREBLER(value));
}

/**
 * Mute (mute=1) or enable sound (mute=0)
 *
 */
int uda1380_mute(int mute)
{
    unsigned int value = uda1380_regs[REG_MUTE];

    if (mute)
        value = value | MUTE_MASTER;
    else
        value = value & ~MUTE_MASTER;

    return uda1380_write_reg(REG_MUTE, value);
}

/* Returns 0 if successful or -1 if some register failed */
int uda1380_set_regs(void)
{
    int i;
    memset(uda1380_regs, 0, sizeof(uda1380_regs));

    /* Initialize all registers */
    for (i=0; i<NUM_DEFAULT_REGS; i++)
    {
        unsigned char reg = uda1380_defaults[i*2+0];
        unsigned short value = uda1380_defaults[i*2+1];

        if (uda1380_write_reg(reg, value) == -1)
            return -1;
    }

    return 0;
}

/* Silently enable / disable audio output */
void uda1380_enable_output(bool enable)
{
    if (enable) {
        uda1380_write_reg(REG_PWR, uda1380_regs[REG_PWR] | PON_HP);
        
        /* Sleep a while, then disable the master mute */
        sleep(HZ/8);
        uda1380_write_reg(REG_MUTE, MUTE_CH2);
    } else {
        uda1380_write_reg(REG_MUTE, MUTE_MASTER);
        uda1380_write_reg(REG_PWR, uda1380_regs[REG_PWR] & ~PON_HP);
    }
}

/* Initialize UDA1380 codec with default register values (uda1380_defaults) */
int uda1380_init(void)
{
    /* RESET signal */
    GPIO_OUT |= (1<<29);
    GPIO_ENABLE |= (1<<29);
    GPIO_FUNCTION |= (1<<29);

    sleep(HZ/100);
    
    GPIO_OUT &= ~(1<<29);
    
    if (uda1380_set_regs() == -1)
        return -1;
    uda1380_balance = 0;
    uda1380_volume = 0x20; /* Taken from uda1380_defaults */

    return 0;
}

/* Nice shutdown of UDA1380 codec */
void uda1380_close(void)
{
    /* First enable mute and sleep a while */
    uda1380_write_reg(REG_MUTE, MUTE_MASTER);
    sleep(HZ/8);

    /* Then power off the rest of the chip */
    uda1380_write_reg(REG_PWR, 0);
    uda1380_write_reg(REG_0, 0);    /* Disable codec    */
}

/**
 * Calling this function enables the UDA1380 to send
 * sound samples over the I2S bus, which is connected
 * to the processor's IIS1 interface. 
 *
 * source_mic: true=record from microphone, false=record from line-in
 */
void uda1380_enable_recording(bool source_mic)
{
   uda1380_write_reg(REG_0, uda1380_regs[REG_0] | EN_ADC);

    if (source_mic)
    {
        uda1380_write_reg(REG_PWR, uda1380_regs[REG_PWR] | PON_LNA | PON_ADCL);
        uda1380_write_reg(REG_ADC, (uda1380_regs[REG_ADC] & VGA_GAIN_MASK) | SEL_LNA | SEL_MIC | EN_DCFIL);   /* VGA_GAIN: 0=0 dB, F=30dB */
        uda1380_write_reg(REG_PGA, 0);
    } else
    {
        uda1380_write_reg(REG_PWR, uda1380_regs[REG_PWR] | PON_PGAL | PON_ADCL | PON_PGAR | PON_ADCR);
        uda1380_write_reg(REG_ADC, EN_DCFIL);
        uda1380_write_reg(REG_PGA, (uda1380_regs[REG_PGA] & PGA_GAIN_MASK) | PGA_GAINL(0) | PGA_GAINR(0)); /* PGA_GAIN: 0=0 dB, F=24dB */
    }

    uda1380_write_reg(REG_I2S,     uda1380_regs[REG_I2S] | I2S_MODE_MASTER);
    uda1380_write_reg(REG_MIX_CTL, MIX_MODE(3));   /* Not sure which mode is the best one.. */

}

/** 
 * Stop sending samples on the I2S bus 
 */
void uda1380_disable_recording(void)
{
    uda1380_write_reg(REG_PGA, MUTE_ADC);
    sleep(HZ/8);
    
    uda1380_write_reg(REG_I2S, I2S_IFMT_IIS);
    uda1380_write_reg(REG_PWR, uda1380_regs[REG_PWR] & ~(PON_LNA | PON_ADCL | PON_ADCR | PON_PGAL | PON_PGAR));
    uda1380_write_reg(REG_0,   uda1380_regs[REG_0] & ~EN_ADC);
    uda1380_write_reg(REG_ADC, SKIP_DCFIL);
}

/**
 * Set recording gain and volume
 * 
 * mic_gain    : range    0 .. 15 ->   0 .. 30 dB gain
 * linein_gain : range    0 .. 15 ->   0 .. 24 dB gain
 * 
 * adc_volume  : range -127 .. 48 -> -63 .. 24 dB gain
 *   note that 0 -> 0 dB gain..
 */
void uda1380_set_recvol(int mic_gain, int linein_gain, int adc_volume)
{
    uda1380_write_reg(REG_DEC_VOL, DEC_VOLL(adc_volume) | DEC_VOLR(adc_volume));
    uda1380_write_reg(REG_PGA, (uda1380_regs[REG_PGA] & ~PGA_GAIN_MASK) | PGA_GAINL(linein_gain) | PGA_GAINR(linein_gain));
    uda1380_write_reg(REG_ADC, (uda1380_regs[REG_ADC] & ~VGA_GAIN_MASK) | VGA_GAIN(mic_gain));
}


/** 
 * Enable or disable recording monitor (so one can listen to the recording)
 * 
 */
void uda1380_set_monitor(int enable)
{
    if (enable)
    {
        /* enable channel 2 */
        uda1380_write_reg(REG_MIX_VOL, (uda1380_regs[REG_MIX_VOL] & 0x00FF) | MIX_VOL_CH_2(0));
        uda1380_write_reg(REG_MUTE, 0);
    } else
    {
        /* mute channel 2 */
        uda1380_write_reg(REG_MUTE, MUTE_CH2);
        uda1380_write_reg(REG_MIX_VOL, (uda1380_regs[REG_MIX_VOL] & 0x00FF) | MIX_VOL_CH_2(0xff));
    }
}

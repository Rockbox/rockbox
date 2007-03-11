/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Driver for AS3514 audio codec
 *
 * Copyright (c) 2007 Daniel Ankers
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
#include "audio.h"
#include "backlight.h"

#include "as3514.h"
#include "i2s.h"
#include "i2c-pp.h"

/* convert tenth of dB volume to master volume register value */
int tenthdb2master(int db)
{
    /* +1..07 to -45.43dB in 1.5dB steps == 32 levels = 5 bits */
    /* 11111 == +1.07dB  (0x1f) = 31) */
    /* 11110 == -0.43dB  (0x1e) = 30) */
    /* 00001 == -43.93dB (0x01) */
    /* 00000 == -45.43dB (0x00) */

    if (db < VOLUME_MIN) {
        return 0x0;
    } else if (db >= VOLUME_MAX) {
        return 0x1f;
    } else {
        return((db-VOLUME_MIN)/15); /* VOLUME_MIN is negative */
    }
}

/* convert tenth of dB volume (-405..60) to mixer volume register value */
int tenthdb2mixer(int db)
{
    /* FIXME: Make this sensible */
    if (db < -405) {
        return 0x0;
    } else if (db >= 60) {
        return 0x1f;
    } else {
        return((db+405)/15);
    }
}

void audiohw_reset(void);

/*
 * Initialise the PP I2C and I2S.
 */
int audiohw_init(void) {
    /* reset I2C */
    i2c_init();

    /* normal outputs for CDI and I2S pin groups */
    DEV_INIT &= ~0x300;

    /*mini2?*/
    outl(inl(0x70000010) & ~0x3000000, 0x70000010);
    /*mini2?*/

    /* device reset */
    DEV_RS |= 0x800;
    DEV_RS &=~0x800;

    /* device enable */
    DEV_EN |= 0x807;

    /* enable external dev clock clocks */
    DEV_EN |= 0x2;

    /* external dev clock to 24MHz */
    outl(inl(0x70000018) & ~0xc, 0x70000018);

    i2s_reset();

    /* Set ADC off, mixer on, DAC on, line out off, line in off, mic off */
    pp_i2c_send(AS3514_I2C_ADDR, AUDIOSET1, 0x60); /* Turn on DAC and mixer */
    pp_i2c_send(AS3514_I2C_ADDR, PLLMODE, 0x04);
    pp_i2c_send(AS3514_I2C_ADDR, DAC_L, 0x50); /* DAC mute off, -16.5dB gain */
    pp_i2c_send(AS3514_I2C_ADDR, DAC_R, 0x10); /* DAC -16.5dB gain to prevent overloading
                                       the headphone amp */
    return 0;
}

void audiohw_postinit(void)
{
}

/* Silently enable / disable audio output */
void audiohw_enable_output(bool enable)
{
    if (enable)
    {
        /* reset the I2S controller into known state */
        i2s_reset();

        pp_i2c_send(AS3514_I2C_ADDR, HPH_OUT_L, 0xc0); /* Mute on, power on */
        audiohw_mute(0);
    } else {
        audiohw_mute(1);
        pp_i2c_send(AS3514_I2C_ADDR, HPH_OUT_L, 0x80); /* Mute on, power off */
    }
}

int audiohw_set_master_vol(int vol_l, int vol_r)
{
    vol_l &= 0x1f;
    vol_r &= 0x1f;
    pp_i2c_send(AS3514_I2C_ADDR, HPH_OUT_R, 0x40 | vol_r);
    pp_i2c_send(AS3514_I2C_ADDR, HPH_OUT_L, 0x40 | vol_l);

    return 0;
}

int audiohw_set_lineout_vol(int vol_l, int vol_r)
{
    pp_i2c_send(AS3514_I2C_ADDR, LINE_OUT_R, 0x40 | vol_r);
    pp_i2c_send(AS3514_I2C_ADDR, LINE_OUT_L, 0x40 | vol_l);

    return 0;
}

int audiohw_set_mixer_vol(int channel1, int channel2)
{
    (void)channel1;
    (void)channel2;

    return 0;
}

/* We are using Linear bass control */
void audiohw_set_bass(int value)
{
    (void)value;
}

void audiohw_set_treble(int value)
{
    (void)value;
}

int audiohw_mute(int mute)
{
    int curr;

    curr = i2c_readbyte(AS3514_I2C_ADDR, HPH_OUT_L);
    if (mute)
    {
        pp_i2c_send(AS3514_I2C_ADDR, HPH_OUT_L, curr | 0x80);
    } else {
        pp_i2c_send(AS3514_I2C_ADDR, HPH_OUT_L, curr & ~(0x80));
    }

    return 0;
}

/* Nice shutdown of WM8758 codec */
void audiohw_close(void)
{
    audiohw_mute(1);
}

/* Change the order of the noise shaper, 5th order is recommended above 32kHz */
void audiohw_set_nsorder(int order)
{
    (void)order;
}

void audiohw_set_sample_rate(int sampling_control)
{
    (void)sampling_control;
    /* TODO: Implement this by adjusting the I2S Master clock */
}

void audiohw_enable_recording(bool source_mic)
{
    (void)source_mic;
    /* TODO */
}

void audiohw_disable_recording(void) {
    /* TODO */
}

void audiohw_set_recvol(int left, int right, int type) {

    (void)left;
    (void)right;
    (void)type;
}

void audiohw_set_monitor(int enable) {

    (void)enable;
}

void audiohw_set_equalizer_band(int band, int freq, int bw, int gain)
{
    (void)band;
    (void)freq;
    (void)bw;
    (void)gain;
}

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
 * Copyright (c) 2007 Christian Gmeiner
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "cpu.h"
#include "debug.h"
#include "system.h"

#include "as3514.h"
#include "i2s.h"
#include "i2c-pp.h"

/* Shadow registers */
int as3514_regs[0x1D];

/*
 * little helper method to set register values.
 * With the help of as3514_regs, we minimize i2c
 * traffic.
 */
static void as3514_write(int reg, int value)
{
    if (pp_i2c_send(AS3514_I2C_ADDR, reg, value) != 2)
    {
        DEBUGF("as3514 error reg=0x%x", reg);
    }
    as3514_regs[reg] = value;
}

/* convert tenth of dB volume to master volume register value */
int tenthdb2master(int db)
{
    /* +6 to -40.43dB in 1.5dB steps == 32 levels = 5 bits */
    /* 11111 == +6dB  (0x1f) = 31) */
    /* 11110 == -4.5dB  (0x1e) = 30) */
    /* 00001 == -39dB (0x01) */
    /* 00000 == -40.5dB (0x00) */

    if (db < VOLUME_MIN) {
        return 0x0;
    } else if (db >= VOLUME_MAX) {
        return 0x1f;
    } else {
        return((db-VOLUME_MIN)/15); /* VOLUME_MIN is negative */
    }
}

void audiohw_reset(void);

/*
 * Initialise the PP I2C and I2S.
 */
int audiohw_init(void)
{
    unsigned int i;

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
    as3514_write(AUDIOSET1, 0x20); /* Turn on DAC */
    as3514_write(AUDIOSET3, 0x5); /* Set HPCM off, ZCU off*/
    as3514_write(HPH_OUT_R, 0xc0 | 0x16); /* set vol and set speaker over-current to 0 */
    as3514_write(HPH_OUT_L, 0x16); /* set default vol for headphone */
    as3514_write(PLLMODE, 0x04);

    /* read all reg values */
    for (i = 0; i < sizeof(as3514_regs); i++) 
    {
        as3514_regs[i] = i2c_readbyte(AS3514_I2C_ADDR, i);
    }

    return 0;
}

void audiohw_postinit(void)
{
}

/* Silently enable / disable audio output */
void audiohw_enable_output(bool enable)
{
    int curr;
    curr = as3514_regs[HPH_OUT_L];

    if (enable)
    {
        /* reset the I2S controller into known state */
        i2s_reset();

        as3514_write(HPH_OUT_L, curr | 0x40); /* power on */
        audiohw_mute(0);
    } else {
        audiohw_mute(1);
        as3514_write(HPH_OUT_L, curr & ~(0x40)); /* power off */
    }
}

int audiohw_set_master_vol(int vol_l, int vol_r)
{
    vol_l &= 0x1f;
    vol_r &= 0x1f;

    /* we are controling dac volume instead of headphone volume,
       as the volume is bigger.
       HDP: 1.07 dB gain
       DAC: 6 dB gain
    */
    as3514_write(DAC_R, vol_r);
    as3514_write(DAC_L, 0x40 | vol_l);

    return 0;
}

int audiohw_set_lineout_vol(int vol_l, int vol_r)
{
    as3514_write(LINE_OUT_R, vol_r);
    as3514_write(LINE_OUT_L, 0x40 | vol_l);

    return 0;
}

int audiohw_mute(int mute)
{
    int curr;
    curr = as3514_regs[HPH_OUT_L];

    if (mute)
    {
        as3514_write(HPH_OUT_L, curr | 0x80);
    } else {
        as3514_write(HPH_OUT_L, curr & ~(0x80));
    }

    return 0;
}

/* Nice shutdown of WM8758 codec */
void audiohw_close(void)
{
    /* mute headphones */
    audiohw_mute(1);

    /* turn off everything */
    as3514_write(AUDIOSET1, 0x0);
}

void audiohw_set_sample_rate(int sampling_control)
{
    (void)sampling_control;
}

void audiohw_enable_recording(bool source_mic)
{
    (void)source_mic;
}

void audiohw_disable_recording(void)
{
}

void audiohw_set_recvol(int left, int right, int type)
{
    (void)left;
    (void)right;
    (void)type;
}

void audiohw_set_monitor(int enable)
{
    (void)enable;
}

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 by Roman Stolyarov
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

#include "config.h"
#include "audio.h"
#include "sound.h"
#include "cpu.h"
#include "system.h"
#include "pcm_sw_volume.h"
#include "cs4398.h"
#include "kernel.h"
#include "button.h"

#define PIN_CS_RST      (32*1+10)
#define PIN_CODEC_PWRON (32*1+13)
#define PIN_AP_MUTE     (32*1+14)
#define PIN_JD_CON      (32*1+16)

static void pop_ctrl(const int val)
{
    if(val)
        __gpio_clear_pin(PIN_JD_CON);
    else
        __gpio_set_pin(PIN_JD_CON);
}

static void amp_enable(const int val)
{
    if(val)
        __gpio_set_pin(PIN_CODEC_PWRON);
    else
        __gpio_clear_pin(PIN_CODEC_PWRON);
}

static void dac_enable(const int val)
{
    if(val)
        __gpio_set_pin(PIN_CS_RST);
    else
        __gpio_clear_pin(PIN_CS_RST);
}

static void ap_mute(bool mute)
{
    if(mute)
        __gpio_clear_pin(PIN_AP_MUTE);
    else
        __gpio_set_pin(PIN_AP_MUTE);
}

static void audiohw_mute(bool mute)
{
    if(mute)
        cs4398_write_reg(CS4398_REG_MUTE, cs4398_read_reg(CS4398_REG_MUTE) | CS4398_MUTE_A | CS4398_MUTE_B);
    else
        cs4398_write_reg(CS4398_REG_MUTE, cs4398_read_reg(CS4398_REG_MUTE) & ~(CS4398_MUTE_A | CS4398_MUTE_B));
}

void audiohw_preinit(void)
{
    cs4398_write_reg(CS4398_REG_MISC, CS4398_CPEN | CS4398_PDN);
    cs4398_write_reg(CS4398_REG_MODECTL, CS4398_FM_SINGLE | CS4398_DEM_NONE | CS4398_DIF_LJUST);
    cs4398_write_reg(CS4398_REG_VOLMIX, CS4398_ATAPI_A_L | CS4398_ATAPI_B_R);
    cs4398_write_reg(CS4398_REG_MUTE, CS4398_MUTEP_LOW);
    cs4398_write_reg(CS4398_REG_VOL_A, 0xff);
    cs4398_write_reg(CS4398_REG_VOL_B, 0xff);
    cs4398_write_reg(CS4398_REG_RAMPFILT, CS4398_ZERO_CROSS | CS4398_SOFT_RAMP);
    cs4398_write_reg(CS4398_REG_MISC, CS4398_CPEN);
}

void audiohw_init(void)
{
    __gpio_as_func1(3*32+12); // BCK
    __gpio_as_func0(3*32+13); // LRCK
    __gpio_as_func2(4*32+5);  // MCLK
    __gpio_as_func0(4*32+7);  // DO

    pop_ctrl(0);
    ap_mute(true);
    amp_enable(0);
    dac_enable(0);

    __gpio_as_output(PIN_JD_CON);
    __gpio_as_output(PIN_AP_MUTE);
    __gpio_as_output(PIN_CODEC_PWRON);
    __gpio_as_output(PIN_CS_RST);

    mdelay(100);
    amp_enable(1);

    /* set AIC clk PLL1 */
    __cpm_select_i2sclk_pll();
    __cpm_select_i2sclk_pll1();

    __cpm_enable_pll_change();
    __cpm_set_i2sdiv(43-1);

    __cpm_start_aic();

    /* Init AIC */
    __i2s_enable_sclk();
    __i2s_external_codec();
    __i2s_select_msbjustified();
    __i2s_as_master();
    __i2s_enable_transmit_dma();
    __i2s_set_transmit_trigger(24);
    __i2s_set_oss_sample_size(16);
    __i2s_enable();

    /* Init DAC */
    dac_enable(1);
    udelay(1);
    audiohw_preinit();
}

static int vol_tenthdb2hw(const int tdb)
{
    if (tdb < CS4398_VOLUME_MIN) {
        return 0xff;
    } else if (tdb > CS4398_VOLUME_MAX) {
        return 0x00;
    } else {
        return (-tdb/5);
    }
}

#ifdef HAVE_LINEOUT_DETECTION
static int real_vol_l, real_vol_r;
#endif

static void jz4760_set_vol(int vol_l, int vol_r)
{
    uint8_t val = cs4398_read_reg(CS4398_REG_MISC) &~ CS4398_FREEZE;
    cs4398_write_reg(CS4398_REG_MISC, val | CS4398_FREEZE);
    cs4398_write_reg(CS4398_REG_VOL_A, vol_tenthdb2hw(vol_l));
    cs4398_write_reg(CS4398_REG_VOL_B, vol_tenthdb2hw(vol_r));
    cs4398_write_reg(CS4398_REG_MISC, val);
}

void audiohw_set_volume(int vol_l, int vol_r)
{
#ifdef HAVE_LINEOUT_DETECTION
    real_vol_l = vol_l;
    real_vol_r = vol_r;

    if (lineout_inserted()) {
       vol_l = 0;
       vol_r = 0;
    }
#endif
    jz4760_set_vol(vol_l, vol_r);
}

void audiohw_set_lineout_volume(int vol_l, int vol_r)
{
    (void)vol_l;
    (void)vol_r;

#ifdef HAVE_LINEOUT_DETECTION
    if (lineout_inserted()) {
       jz4760_set_vol(0, 0);
    } else {
       jz4760_set_vol(real_vol_l, real_vol_r);
    }
#endif
}

void audiohw_set_filter_roll_off(int value)
{
    if (value == 0) {
        cs4398_write_reg(CS4398_REG_RAMPFILT, cs4398_read_reg(CS4398_REG_RAMPFILT) & ~CS4398_FILT_SEL);
    } else {
        cs4398_write_reg(CS4398_REG_RAMPFILT, cs4398_read_reg(CS4398_REG_RAMPFILT) | CS4398_FILT_SEL);
    }
}

void pll1_init(unsigned int freq);
void audiohw_set_frequency(int fsel)
{
    unsigned int  pll1_speed;
    unsigned char mclk_div, bclk_div, func_mode;

    switch(fsel)
    {
        case HW_FREQ_8:
            pll1_speed = 426000000;
            mclk_div = 52;
            bclk_div = 16;
            func_mode = 0;
            break;
        case HW_FREQ_11:
            pll1_speed = 508000000;
            mclk_div = 45;
            bclk_div = 16;
            func_mode = 0;
            break;
        case HW_FREQ_12:
            pll1_speed = 516000000;
            mclk_div = 42;
            bclk_div = 16;
            func_mode = 0;
            break;
        case HW_FREQ_16:
            pll1_speed = 426000000;
            mclk_div = 52;
            bclk_div = 8;
            func_mode = 0;
            break;
        case HW_FREQ_22:
            pll1_speed = 508000000;
            mclk_div = 45;
            bclk_div = 8;
            func_mode = 0;
            break;
        case HW_FREQ_24:
            pll1_speed = 516000000;
            mclk_div = 42;
            bclk_div = 8;
            func_mode = 0;
            break;
        case HW_FREQ_32:
            pll1_speed = 426000000;
            mclk_div = 52;
            bclk_div = 4;
            func_mode = 0;
            break;
        case HW_FREQ_44:
            pll1_speed = 508000000;
            mclk_div = 45;
            bclk_div = 4;
            func_mode = 0;
            break;
        case HW_FREQ_48:
            pll1_speed = 516000000;
            mclk_div = 42;
            bclk_div = 4;
            func_mode = 0;
            break;
        case HW_FREQ_64:
            pll1_speed = 426000000;
            mclk_div = 52;
            bclk_div = 2;
            func_mode = 1;
            break;
        case HW_FREQ_88:
            pll1_speed = 508000000;
            mclk_div = 45;
            bclk_div = 2;
            func_mode = 1;
            break;
        case HW_FREQ_96:
            pll1_speed = 516000000;
            mclk_div = 42;
            bclk_div = 2;
            func_mode = 1;
            break;
        default:
            return;
    }

    ap_mute(true);
    __i2s_stop_bitclk();

    /* 0 = Single-Speed Mode (<50KHz);
       1 = Double-Speed Mode (50-100KHz);
       2 = Quad-Speed Mode;  (100-200KHz) */
    cs4398_write_reg(CS4398_REG_MODECTL, (cs4398_read_reg(CS4398_REG_MODECTL) & ~CS4398_FM_MASK) | func_mode);
    if (func_mode == 2)
        cs4398_write_reg(CS4398_REG_MISC, cs4398_read_reg(CS4398_REG_MISC) | CS4398_MCLKDIV2);
    else
        cs4398_write_reg(CS4398_REG_MISC, cs4398_read_reg(CS4398_REG_MISC) & ~CS4398_MCLKDIV2);

    pll1_init(pll1_speed);
    __cpm_enable_pll_change();
    __cpm_set_i2sdiv(mclk_div-1);
    __i2s_set_i2sdiv(bclk_div-1);
    __i2s_start_bitclk();
    mdelay(20);
    ap_mute(false);
}

void audiohw_postinit(void)
{
    sleep(HZ);
    audiohw_mute(false);
    ap_mute(false);
    pop_ctrl(1);
}

void audiohw_close(void)
{
    pop_ctrl(0);
    sleep(HZ/10);
    ap_mute(true);
    audiohw_mute(true);
    amp_enable(0);
    dac_enable(0);
    __i2s_disable();
    __cpm_stop_aic();
    sleep(HZ);
    pop_ctrl(1);
}

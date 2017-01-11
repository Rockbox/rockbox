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
#include "stdlib.h"
#include "string.h"
#include "pwm-imx233.h"
#include "clkctrl-imx233.h"
#include "pinctrl-imx233.h"

#include "regs/pwm.h"

/* fake field for simpler programming */
#define BP_PWM_CTRL_PWMx_ENABLE(x)  (x)
#define BM_PWM_CTRL_PWMx_ENABLE(x)  (1 << (x))
#define BF_PWM_CTRL_PWMx_ENABLE(x, v)   (((v) << BP_PWM_CTRL_PWMx_ENABLE(x)) & BM_PWM_CTRL_PWMx_ENABLE(x))
#define BFM_PWM_CTRL_PWMx_ENABLE(x, v)  BM_PWM_CTRL_PWMx_ENABLE(x)

/* list of divisors + register value by increasing order of divisors */
static int pwm_cdiv_table[] =
{
#define DIV(d) [BV_PWM_PERIODn_CDIV__DIV_##d] = d
    DIV(1), DIV(2), DIV(4), DIV(8), DIV(16), DIV(64), DIV(256), DIV(1024)
#undef DIV
};

void imx233_pwm_init(void)
{
    imx233_reset_block(&HW_PWM_CTRL);
    imx233_clkctrl_enable(CLK_PWM, true);
}

bool imx233_pwm_is_enabled(int channel)
{
    return BF_RD(PWM_CTRL, PWMx_ENABLE(channel));
}

void imx233_pwm_enable(int channel, bool enable)
{
    if(enable)
    {
        /* claim pin */
        imx233_pinctrl_setup_vpin(VPIN_PWM(channel), "pwm", PINCTRL_DRIVE_4mA, false);
        BF_SET(PWM_CTRL, PWMx_ENABLE(channel));
    }
    else
    {
        BF_CLR(PWM_CTRL, PWMx_ENABLE(channel));
        /* stop claiming the pin */
        imx233_pinctrl_release(VPIN_UNPACK_BANK(VPIN_PWM(channel)),
            VPIN_UNPACK_PIN(VPIN_PWM(channel)), "pwm");
    }
}

void imx233_pwm_setup(int channel, int period, int cdiv, int active,
    int active_state, int inactive, int inactive_state)
{
    /* stop */
    bool enable = imx233_pwm_is_enabled(channel);
    if(enable)
        imx233_pwm_enable(channel, false);
    /* watch the order ! active THEN period
     * NOTE: the register value is period-1 */
    BF_WR_ALL(PWM_ACTIVEn(channel), ACTIVE(active), INACTIVE(inactive));
    BF_WR_ALL(PWM_PERIODn(channel), PERIOD(period - 1),
        ACTIVE_STATE(active_state), INACTIVE_STATE(inactive_state), CDIV(cdiv));
    /* restore */
    imx233_pwm_enable(channel, enable);
}

void imx233_pwm_lookup_freq(int freq, int min_period, int *out_period, int *out_cdiv)
{
    /* find best divisor */
    int best_freq_err = freq;
    int xtal_freq = imx233_clkctrl_get_freq(CLK_XTAL) * 1000;

    for(unsigned cdiv = 0; cdiv < ARRAYLEN(pwm_cdiv_table); cdiv++)
    {
        /* compute best period (we have two rounding choices) */
        int p = xtal_freq / (pwm_cdiv_table[cdiv] * freq);
        for(int period = p; period <= p + 1; period++)
        {
            /* avoid forbidden periods */
            if(p < min_period || p > IMX233_PWM_MAX_PERIOD)
                continue;
            /* compute actual frequency and compare with best obtained so far */
            int f = xtal_freq / (pwm_cdiv_table[cdiv] * period);
            if(ABS(freq - f) <= best_freq_err)
            {
                *out_period = period;
                *out_cdiv = cdiv;
                best_freq_err = ABS(freq - f);
            }
        }
    }
}

void imx233_pwm_setup_simple(int channel, int freq, int duty_cycle)
{
    int period, cdiv;
    imx233_pwm_lookup_freq(freq, 100, &period, &cdiv);
    int inactive = (period * duty_cycle) / 100;
    imx233_pwm_setup(channel, period, cdiv, 0, BV_PWM_PERIODn_ACTIVE_STATE__1,
        inactive, BV_PWM_PERIODn_INACTIVE_STATE__0);
}

struct imx233_pwm_info_t imx233_pwm_get_info(int channel)
{
#define ENTRY(mode, name, val) [BV_PWM_PERIODn_##mode##_STATE__##name] = val
    static char active_state[] =
    {
        ENTRY(ACTIVE, 0, '0'), ENTRY(ACTIVE, 1, '1'), ENTRY(ACTIVE, HI_Z, 'Z')
    };
    static char inactive_state[] =
    {
        ENTRY(INACTIVE, 0, '0'), ENTRY(INACTIVE, 1, '1'), ENTRY(INACTIVE, HI_Z, 'Z')
    };
#undef ENTRY
    struct imx233_pwm_info_t info;
    memset(&info, 0, sizeof(info));
    info.enabled = imx233_pwm_is_enabled(channel);
    info.cdiv = pwm_cdiv_table[BF_RD(PWM_PERIODn(channel), CDIV)];
    info.period = BF_RD(PWM_PERIODn(channel), PERIOD) + 1;
    info.active = BF_RD(PWM_ACTIVEn(channel), ACTIVE);
    info.inactive = BF_RD(PWM_ACTIVEn(channel), INACTIVE);
    info.active_state = active_state[BF_RD(PWM_PERIODn(channel), ACTIVE_STATE)];
    info.inactive_state = inactive_state[BF_RD(PWM_PERIODn(channel), INACTIVE_STATE)];
    return info;
}

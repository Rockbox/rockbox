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
#include "pwm-imx233.h"
#include "clkctrl-imx233.h"
#include "pinctrl-imx233.h"

void imx233_pwm_init(void)
{
    imx233_reset_block(&HW_PWM_CTRL);
    imx233_clkctrl_enable_xtal(XTAM_PWM, true);
}

bool imx233_pwm_is_channel_enable(int channel)
{
    return BF_RD(PWM_CTRL, PWMx_ENABLE(channel));
}

void imx233_pwm_enable_channel(int channel, bool enable)
{
    if(enable)
        BF_SET(PWM_CTRL, PWMx_ENABLE(channel));
    else
        BF_CLR(PWM_CTRL, PWMx_ENABLE(channel));
}

void imx233_pwm_setup_channel(int channel, int period, int cdiv, int active,
    int active_state, int inactive, int inactive_state)
{
    /* stop */
    bool enable = imx233_pwm_is_channel_enable(channel);
    if(enable)
        imx233_pwm_enable_channel(channel, false);
    /* setup pin */
    imx233_pinctrl_setup_vpin(VPIN_PWM(channel), "pwm", PINCTRL_DRIVE_4mA, false);
    /* watch the order ! active THEN period */
    HW_PWM_ACTIVEn(channel) = BF_OR2(PWM_ACTIVEn, ACTIVE(active), INACTIVE(inactive));
    HW_PWM_PERIODn(channel) = BF_OR4(PWM_PERIODn, PERIOD(period - 1),
        ACTIVE_STATE(active_state), INACTIVE_STATE(inactive_state), CDIV(cdiv));
    /* restore */
    imx233_pwm_enable_channel(channel, enable);
}

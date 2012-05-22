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
    return HW_PWM_CTRL & HW_PWM_CTRL__PWMx_ENABLE(channel);
}

void imx233_pwm_enable_channel(int channel, bool enable)
{
    if(enable)
        __REG_SET(HW_PWM_CTRL) = HW_PWM_CTRL__PWMx_ENABLE(channel);
    else
        __REG_CLR(HW_PWM_CTRL) = HW_PWM_CTRL__PWMx_ENABLE(channel);
}

void imx233_pwm_setup_channel(int channel, int period, int cdiv, int active,
    int active_state, int inactive, int inactive_state)
{
    /* stop */
    bool enable = imx233_pwm_is_channel_enable(channel);
    if(enable)
        imx233_pwm_enable_channel(channel, false);
    /* setup pin */
    imx233_pinctrl_acquire_pin(IMX233_PWM_PIN_BANK(channel),
        IMX233_PWM_PIN(channel), "pwm");
    imx233_set_pin_function(IMX233_PWM_PIN_BANK(channel), IMX233_PWM_PIN(channel),
        PINCTRL_FUNCTION_MAIN);
    imx233_set_pin_drive_strength(IMX233_PWM_PIN_BANK(channel), IMX233_PWM_PIN(channel),
        PINCTRL_DRIVE_4mA);
    /* watch the order ! active THEN period */
    HW_PWM_ACTIVEx(channel) = active << HW_PWM_ACTIVEx__ACTIVE_BP |
        inactive << HW_PWM_ACTIVEx__INACTIVE_BP;
    HW_PWM_PERIODx(channel) = period | active_state << HW_PWM_PERIODx__ACTIVE_STATE_BP |
        inactive_state << HW_PWM_PERIODx__INACTIVE_STATE_BP |
        cdiv << HW_PWM_PERIODx__CDIV_BP;
    /* restore */
    imx233_pwm_enable_channel(channel, enable);
}

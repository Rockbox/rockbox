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
#ifndef __PWM_IMX233_H__
#define __PWM_IMX233_H__

#include "system.h"

#define IMX233_PWM_MAX_PERIOD   (1 << 16)

#define IMX233_PWM_NR_CHANNELS  5

void imx233_pwm_init(void);
void imx233_pwm_setup(int channel, int period, int cdiv, int active,
    int active_state, int inactive, int inactive_state);
/* helper function to compute adequate divisor and period, given a minimum
 * period resolution (for active/inactive) */
void imx233_pwm_lookup_freq(int freq, int min_period, int *period, int *cdiv);
/* simple setup with active 1, inactive 0, duty cycle in percent */
void imx233_pwm_setup_simple(int channel, int freq, int duty_cycle);
void imx233_pwm_enable(int channel, bool enable);
bool imx233_pwm_is_enabled(int channel);

struct imx233_pwm_info_t
{
    bool enabled;
    int cdiv;
    int period;
    int inactive, active;
    char inactive_state, active_state; // '0', '1' or 'Z'
};

struct imx233_pwm_info_t imx233_pwm_get_info(int channel);

#endif /* __PWM_IMX233_H__ */

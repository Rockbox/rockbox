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

#include "regs/regs-pwm.h"

/* fake field for simpler programming */
#define BP_PWM_CTRL_PWMx_ENABLE(x)  (x)
#define BM_PWM_CTRL_PWMx_ENABLE(x)  (1 << (x))

#define IMX233_PWM_MAX_PERIOD   (1 << 16)

#define IMX233_PWM_NR_CHANNELS  5

#define IMX233_PWM_PIN_BANK(channel)    1
#define IMX233_PWM_PIN(channel) (26 + (channel))

void imx233_pwm_init(void);
void imx233_pwm_setup_channel(int channel, int period, int cdiv, int active,
    int active_state, int inactive, int inactive_state);
void imx233_pwm_enable_channel(int channel, bool enable);
bool imx233_pwm_is_channel_enable(int channel);

#endif /* __PWM_IMX233_H__ */

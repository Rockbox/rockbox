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

#define HW_PWM_BASE     0x80064000 

#define HW_PWM_CTRL         (*(volatile uint32_t *)(HW_PWM_BASE + 0x0))
#define HW_PWM_CTRL__PWMx_ENABLE(x) (1 << (x))

#define HW_PWM_ACTIVEx(x)   (*(volatile uint32_t *)(HW_PWM_BASE + 0x10 + (x) * 0x20))
#define HW_PWM_ACTIVEx__ACTIVE_BP   0
#define HW_PWM_ACTIVEx__ACTIVE_BM   0xffff
#define HW_PWM_ACTIVEx__INACTIVE_BP 16
#define HW_PWM_ACTIVEx__INACTIVE_BM 0xffff0000

#define HW_PWM_PERIODx(x)   (*(volatile uint32_t *)(HW_PWM_BASE + 0x20 + (x) * 0x20))
#define HW_PWM_PERIODx__PERIOD_BP   0
#define HW_PWM_PERIODx__PERIOD_BM   0xffff
#define HW_PWM_PERIODx__ACTIVE_STATE_BP     16
#define HW_PWM_PERIODx__ACTIVE_STATE_BM     (0x3 << 16)
#define HW_PWM_PERIODx__INACTIVE_STATE_BP   18
#define HW_PWM_PERIODx__INACTIVE_STATE_BM   (0x3 << 18)
#define HW_PWM_PERIODx__CDIV_BP     20
#define HW_PWM_PERIODx__CDIV_BM     (0x7 << 20)
#define HW_PWM_PERIODx__CDIV__DIV_1     0
#define HW_PWM_PERIODx__CDIV__DIV_2     1
#define HW_PWM_PERIODx__CDIV__DIV_4     2
#define HW_PWM_PERIODx__CDIV__DIV_8     3
#define HW_PWM_PERIODx__CDIV__DIV_16    4
#define HW_PWM_PERIODx__CDIV__DIV_64    5
#define HW_PWM_PERIODx__CDIV__DIV_256   6
#define HW_PWM_PERIODx__CDIV__DIV_1024  7

#define HW_PWM_PERIODx__STATE__HI_Z 0
#define HW_PWM_PERIODx__STATE__LOW  2
#define HW_PWM_PERIODx__STATE__HIGH 3

#define IMX233_PWM_PIN_BANK(channel)    1
#define IMX233_PWM_PIN(channel) (26 + (channel))

void imx233_pwm_init(void);
void imx233_pwm_setup_channel(int channel, int period, int cdiv, int active,
    int active_state, int inactive, int inactive_state);
void imx233_pwm_enable_channel(int channel, bool enable);
bool imx233_pwm_is_channel_enable(int channel);

#endif /* __PWM_IMX233_H__ */

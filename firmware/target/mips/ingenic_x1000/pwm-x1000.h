/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef __PWM_X1000_H__
#define __PWM_X1000_H__

/* Usage:
 * - There are 5 PWM channels (0-4) corresponding to TCUs 0-4
 * - Call pwm_init(n) before using channel n
 * - Call pwm_set_period() to change the period and duty cycle at any time
 * - Call pwm_enable() and pwm_disable() to turn the output on and off
 * - Don't allow two threads to control the same channel at the same time
 * - Don't call pwm_init(), pwm_enable(), or pwm_disable() from an interrupt
 *
 * After calling pwm_init(), the channel is essentially in a disabled state so
 * you will need to call pwm_set_period() and then pwm_enable() to turn it on.
 * Don't alter the channel's TCU or GPIO pin state after calling pwm_init().
 *
 * After calling pwm_disable(), it is safe to use the channel's TCU or GPIO pin
 * for some other purpose, but you must call pwm_init() before you can use the
 * channel as PWM output again.
 */

extern void pwm_init(int chn);
extern void pwm_set_period(int chn, int period_ns, int duty_ns);
extern void pwm_enable(int chn);
extern void pwm_disable(int chn);

#endif /* __PWM_X1000_H__ */

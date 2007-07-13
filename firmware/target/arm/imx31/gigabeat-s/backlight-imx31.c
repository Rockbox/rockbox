/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "backlight-target.h"
#include "backlight.h"
#include "lcd.h"
#include "power.h"

static void led_control_service(void);

bool __backlight_init(void)
{
    return true;
}

void __backlight_on(void)
{
}

void __backlight_off(void)
{
}

/* Assumes that the backlight has been initialized */
void __backlight_set_brightness(int brightness)
{
    (void)brightness;
}

/* only works if the buttonlight mode is set to triggered mode */
void __buttonlight_trigger(void)
{
}

/* map the mode from the command into the state machine entries */
void __buttonlight_mode(int mode)
{
    (void)mode;
}

/* led_control_service runs in interrupt context - be brief!
 * This service is called once per interrupt timer tick - 100 times a second.
 *
 * There should be at most only one i2c operation per call - if more are need
 *  the calls should be spread across calls.
 *
 * Putting all led servicing in one thread means that we wont step on any
 * i2c operations - they are all serialized here in the ISR tick. It also
 * insures that we get called at equal timing for good visual effect.
 */
static void led_control_service(void)
{
}

void __button_backlight_on(void)
{
}

void __button_backlight_off(void)
{
}

void __button_backlight_dim(bool dim_now)
{
    (void)dim_now;
}

void __backlight_dim(bool dim_now)
{
    (void)dim_now;
}

void __buttonlight_set_brightness(int brightness)
{
    (void)brightness;
}

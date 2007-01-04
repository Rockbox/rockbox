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
#include "sc606-meg-fx.h"


static void backlight_fade_service(void);
static unsigned short backlight_brightness;
static unsigned short backlight_fading;
static unsigned short backlight_current;
static unsigned short backlight_target;
static unsigned short time_til_fade;
static unsigned short fade_interval;


static int confval = SC606_LOW_FREQ;



void __backlight_init(void)
{
    backlight_fading = false;

    /* current is from settings */
    backlight_current = 50;
    
    /* put the fade tick on the list */
    tick_add_task(backlight_fade_service);
}



void __backlight_on(void)
{
    confval |= (SC606_LED_A1 | SC606_LED_A2);
    sc606_write(SC606_REG_CONF, confval);
}



void __backlight_off(void)
{
    confval &= ~(SC606_LED_A1 | SC606_LED_A2);
    sc606_write(SC606_REG_CONF, confval);
}



/* Assumes that the backlight has been initialized */
void __backlight_set_brightness(int brightness)
{
    /* stop the interrupt from messing us up */
    backlight_fading = false;

    backlight_brightness = brightness;

    /* only set the brightness if it is different from the current */
    if (backlight_brightness != backlight_current)
    {
        backlight_target = brightness;
        fade_interval = time_til_fade = 1;
        backlight_fading = true;
    }
}



static void backlight_fade_service(void)
{
    if (!backlight_fading || --time_til_fade) return;
            
    if (backlight_target > backlight_current)
    {
        backlight_current++;
    }
    else
    {
        backlight_current--;
    }
        
    /* The SC606 LED driver can set the brightness in 64 steps */
    sc606_write(SC606_REG_A, backlight_current);


    /* have we hit the target? */
    if (backlight_current == backlight_target)
    {
        backlight_fading = false;
    }
    else
    {
        time_til_fade = fade_interval;
    }

}





void __backlight_dim(bool dim_now)
{
    int target;
    
    /* dont let the interrupt tick happen */
    backlight_fading = false;

    target = (dim_now == true) ? 0 : backlight_brightness;
    
    /* only try and fade if the target is different */
    if (backlight_current != target)
    {
        backlight_target = target;
        
        if (backlight_current > backlight_target) 
        {
            fade_interval = 4;
        }
        else
        {
            fade_interval = 1;
        }

        /* let the tick work */
        time_til_fade = fade_interval;
        backlight_fading = true;
    }
            
}


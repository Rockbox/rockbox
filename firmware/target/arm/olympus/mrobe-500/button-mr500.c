/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: $
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "button.h"
#include "kernel.h"
#include "backlight.h"
#include "adc.h"
#include "system.h"
#include "backlight-target.h"

static int const remote_buttons[] =
{
    BUTTON_NONE,        /* Headphones connected - remote disconnected */
    BUTTON_RC_PLAY,
    BUTTON_RC_DSP,
    BUTTON_RC_REW,
    BUTTON_RC_FF,
    BUTTON_RC_VOL_UP,
    BUTTON_RC_VOL_DOWN,
    BUTTON_NONE,        /* Remote control attached - no buttons pressed */
    BUTTON_NONE,        /* Nothing in the headphone socket */
};

void button_init_device(void)
{
    /* Power, Remote Play & Hold switch */
}

inline bool button_hold(void)
{
    return false;
}

int button_read_device(void)
{
    return 0;
}

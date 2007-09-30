/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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
#include "uart-target.h"

#define BUTTON_TIMEOUT 50

#define BUTTON_START_BYTE 0xF0
#define BUTTON_START_BYTE2 0xF4 /* not sure why, but sometimes you get F0 or F4, */
                                /* but always the same one for the session? */

void button_init_device(void)
{
    /* GIO is the power button, set as input */
    IO_GIO_DIR0 |= 0x01;
}

inline bool button_hold(void)
{
    return false;
}

int button_read_device(void)
{
    char data[5], c;
    int i = 0;
    int btn = BUTTON_NONE;
    
    if ((IO_GIO_BITSET0&0x01) == 0)
        btn |= BUTTON_POWER;
        
    uart1_heartbeat();
    while (uartAvailable())
    {
        if (uart1_getch(&c))
        {
            if (i && (data[0] == BUTTON_START_BYTE || data[0] == BUTTON_START_BYTE2))
            {
                data[i++] = c;
            }
            else if (c == BUTTON_START_BYTE ||
                     c == BUTTON_START_BYTE2)
            {
                data[0] = c;
                i = 1;
            }
            
            if (i == 5)
            {
                if (data[1]& (1<<7))
                    btn |= BUTTON_RC_HEART;
                if (data[1]& (1<<6))
                    btn |= BUTTON_RC_MODE;
                if (data[1]& (1<<5))
                    btn |= BUTTON_RC_VOL_DOWN;
                if (data[1]& (1<<4))
                    btn |= BUTTON_RC_VOL_UP;
                if (data[1]& (1<<3))
                    btn |= BUTTON_RC_REW;
                if (data[1]& (1<<2))
                    btn |= BUTTON_RC_FF;
                if (data[1]& (1<<1))
                    btn |= BUTTON_RC_DOWN;
                if (data[1]& (1<<0))
                    btn |= BUTTON_RC_PLAY;
                break;
            }
        }
    }
    return btn;
}

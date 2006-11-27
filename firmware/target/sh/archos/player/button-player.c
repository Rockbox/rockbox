/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"
#include "system.h"
#include "button.h"
#include "backlight.h"
#include "adc.h"

/*
 Player hardware button hookup
 =============================

 Player
 ------
 LEFT:   AN0
 MENU:   AN1
 RIGHT:  AN2
 PLAY:   AN3

 STOP:   PA11
 ON:     PA5

 All buttons are low active
*/

void button_init_device(void)
{
    /* set PA5 and PA11 as input pins */
    PACR1 &= 0xff3f;  /* PA11MD = 00 */
    PACR2 &= 0xfbff;  /* PA5MD = 0 */
    PAIOR &= ~0x0820; /* Inputs */
}

int button_read_device(void)
{
    int btn = BUTTON_NONE;
    int data;

    /* buttons are active low */
    if (adc_read(0) < 0x180)
        btn = BUTTON_LEFT;
    if (adc_read(1) < 0x180)
        btn |= BUTTON_MENU;
    if (adc_read(2) < 0x180)
        btn |= BUTTON_RIGHT;
    if (adc_read(3) < 0x180)
        btn |= BUTTON_PLAY;

    /* check port A pins for ON and STOP */
    data = PADR;
    if ( !(data & 0x0020) )
        btn |= BUTTON_ON;
    if ( !(data & 0x0800) )
        btn |= BUTTON_STOP;
        
    return btn;
}

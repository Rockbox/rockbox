/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "config.h"

#include <led.h>

#define turn_on() \
  set_bit (LEDB,PBDR_ADDR+1)

#define turn_off() \
  clear_bit (LEDB,PBDR_ADDR+1)

#define start_timer() \
  set_bit (2,TSTR_ADDR)

#define stop_timer() \
  clear_bit (2,TSTR_ADDR)

#define eoi(subinterrupt) \
  clear_bit (subinterrupt,TSR2_ADDR)

#define set_volume(volume) \
  GRA2 = volume & 0x7FFF


void led_set_volume (unsigned short volume)
{
    volume <<= 10;
    if (volume == 0)
	led_turn_off ();
    else if (volume == 0x8000) 
	led_turn_on ();
    else
    {
	set_volume (volume);
        start_timer ();
    }
}

#pragma interrupt
void IMIA2 (void)
{
    turn_off ();
    eoi (0);
}

#pragma interrupt
void OVI2 (void)
{
    turn_on ();
    eoi (2);
}
    

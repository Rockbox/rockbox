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

#ifndef __LED_H__
#define __LED_H__

#include <sh7034.h>
#include <system.h>

#define LEDB 6 /* PB6 : red LED */

static inline void led_turn_off (void)
  {
    clear_bit (LEDB,PBDR+1);
    clear_bit (2,ITUTSTR);
  }

static inline void led_turn_on (void)
  {
    set_bit (LEDB,PBDR+1);
    set_bit (2,ITUTSTR);
  }

static inline void led_toggle (void)
  {
    toggle_bit (LEDB,PBDR+1);
  }

extern void led_set_volume (unsigned short volume);
extern void led_setup (void);

#endif



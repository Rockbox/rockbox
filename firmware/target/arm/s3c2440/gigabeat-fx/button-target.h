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
#ifndef _BUTTON_TARGET_H_
#define _BUTTON_TARGET_H_

#include <stdbool.h>
#include "config.h"

#define HAS_BUTTON_HOLD

bool button_hold(void);
void button_init_device(void);
int button_read_device(void);

/* Toshiba Gigabeat specific button codes */

#define BUTTON_POWER        0x00000001
#define BUTTON_MENU         0x00000002

#define BUTTON_LEFT         0x00000004
#define BUTTON_RIGHT        0x00000008
#define BUTTON_UP           0x00000010
#define BUTTON_DOWN         0x00000020

#define BUTTON_VOL_UP       0x00000040
#define BUTTON_VOL_DOWN     0x00000080

#define BUTTON_SELECT       0x00000100
#define BUTTON_A            0x00000200


/* Toshiba Gigabeat specific remote button ADC values */
/* The remote control uses ADC 1 to emulate button pushes
    Reading (approx)     Button      HP plugged in?      Remote plugged in?
         0                 N/A            Yes                  No
        125             Play/Pause      Cant tell             Yes
        241             Speaker+        Cant tell             Yes
        369             Rewind          Cant tell             Yes
        492             Fast Fwd        Cant tell             Yes
        616             Vol +           Cant tell             Yes
        742             Vol -           Cant tell             Yes
        864             None            Cant tell             Yes
       1023              N/A               No                  No
*/

/* 
    Notes:

    Buttons on the remote are translated into equivalent button presses just
    as if you were pressing them on the Gigabeat itself.
    
    We cannot tell if the hold is asserted on the remote. The Hold function on 
    the remote is to block the output of the buttons changing.

    Only one button can be sensed at a time. If another is pressed, the button
    with the lowest reading is dominant. So, if Rewind and Vol + are pressed
    at the same time, Rewind value is the one that is read.
*/
    



#define BUTTON_MAIN (BUTTON_POWER|BUTTON_MENU|BUTTON_LEFT|BUTTON_RIGHT\
                |BUTTON_UP|BUTTON_DOWN|BUTTON_VOL_UP|BUTTON_VOL_DOWN\
                |BUTTON_SELECT|BUTTON_A)


#define BUTTON_REMOTE 0

#define POWEROFF_BUTTON BUTTON_POWER
#define POWEROFF_COUNT 10

#endif /* _BUTTON_TARGET_H_ */

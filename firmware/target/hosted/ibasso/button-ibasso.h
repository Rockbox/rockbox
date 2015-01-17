/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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


#ifndef _BUTTON_IBASSO_H_
#define _BUTTON_IBASSO_H_


#include <sys/types.h>


/* /dev/input/event0 */
#define EVENT_CODE_BUTTON_PWR      116
#define EVENT_CODE_BUTTON_PWR_LONG 117

/* /dev/input/event1 */
#define EVENT_CODE_BUTTON_VOLPLUS  158
#define EVENT_CODE_BUTTON_VOLMINUS 159
#define EVENT_CODE_BUTTON_REV      160
#define EVENT_CODE_BUTTON_PLAY     161
#define EVENT_CODE_BUTTON_NEXT     162

#define EVENT_VALUE_BUTTON_PRESS   1
#define EVENT_VALUE_BUTTON_RELEASE 0


/*
    Handle hardware button events.
    code: Input event code.
    value: Input event value.
    last_btns: Last known pressed buttons.
    Returns: Currently pressed buttons as bitmask (BUTTON_ values in button-target.h).
*/
int handle_button_event(__u16 code, __s32 value, int last_btns);


/* Clean up the button device handler. */
void button_close_device(void);


#endif

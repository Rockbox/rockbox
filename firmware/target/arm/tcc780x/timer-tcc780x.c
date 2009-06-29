/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2008 by Rob Purchase
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

#include "config.h"
#include "cpu.h"
#include "system.h"
#include "timer.h"
#include "logf.h"

static const int prescale_shifts[] = {1, 2, 3, 4, 5, 10, 12};

bool timer_set(long cycles, bool start)
{
    bool found = false;
 
    int prescale_option = 0;
    int actual_cycles = 0;
 
    /* Use the first prescale that fits Timer4's 20-bit counter */
    while (!found && prescale_option < 7)
    {
        actual_cycles = cycles >> prescale_shifts[prescale_option];
 
        if (actual_cycles < 0x100000)
            found = true;
        else
            prescale_option++;
    }
 
    if (!found)
        return false;

    /* Stop the timer and set new prescale & ref count */
    TCFG(4) &= ~TCFG_EN;
    TCFG(4) = prescale_option << TCFG_SEL;
    TREF(4) = actual_cycles;
    
    if (start && pfn_unregister != NULL)
    {
        pfn_unregister();
        pfn_unregister = NULL;
    }
    
    return true;
}

bool timer_start(void)
{
    int oldstatus = disable_interrupt_save(IRQ_STATUS);
    
    TCFG(4) |= TCFG_CLEAR | TCFG_IEN | TCFG_EN;
    
    restore_interrupt(oldstatus);
    
    return true;
}

void timer_stop(void)
{
    int oldstatus = disable_interrupt_save(IRQ_STATUS);
    
    TCFG(4) &= ~TCFG_EN;
    
    restore_interrupt(oldstatus);
}

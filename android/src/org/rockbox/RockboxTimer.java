/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
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

package org.rockbox;

import java.util.Timer;
import java.util.TimerTask;

import android.content.Context;
import android.util.Log;

public class RockboxTimer extends Timer 
{    
    private class RockboxTimerTask extends TimerTask {
        private RockboxTimer timer;
        public RockboxTimerTask(RockboxTimer parent) 
        {
            super();
            timer = parent;
        }

        @Override
        public void run() 
        {
            timerTask();
            synchronized(timer) { 
                timer.notify(); 
            }
        }
    }
    
    public RockboxTimer(Context c, long period_inverval_in_ms)
    {
        super("tick timer");
        schedule(new RockboxTimerTask(this), 0, period_inverval_in_ms);
    }

    @SuppressWarnings("unused")
    private void LOG(CharSequence text)
    {
        Log.d("Rockbox", (String) text);    
    }


    /* methods called from native, keep them simple */    
    public void java_wait_for_interrupt()
    {
        synchronized(this) 
        {
            try {
                this.wait();
            } catch (InterruptedException e) {
                /* Not an error: wakeup and return */
            }
        }
    }
    public native void timerTask();
}

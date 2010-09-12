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
import android.telephony.TelephonyManager;
import android.util.Log;

public class RockboxTimer extends Timer 
{
    RockboxTimerTask task;
    long interval;
    
    private class RockboxTimerTask extends TimerTask {
        private RockboxTimer timer;
        private TelephonyManager tm;
        private int last_state;
        public RockboxTimerTask(RockboxService s, RockboxTimer parent) 
        {
            super();
            timer = parent;
            tm = (TelephonyManager)s.getSystemService(Context.TELEPHONY_SERVICE);
            last_state = tm.getCallState();
        }

        @Override
        public void run() 
        {
            timerTask();
            int state = tm.getCallState();
            if (state != last_state)
            {
                switch (state) 
                {
                    case TelephonyManager.CALL_STATE_IDLE:
                        postCallHungUp();
                        break;
                    case TelephonyManager.CALL_STATE_RINGING:
                        postCallIncoming();
                    default:
                        break;
                }
                last_state = state;
            }
            synchronized(timer) { 
                timer.notify(); 
            }
        }
    }
    
    public RockboxTimer(RockboxService instance, long period_inverval_in_ms)
    {
        super("tick timer");
        task = new RockboxTimerTask(instance, this);
        schedule(task, 0, period_inverval_in_ms);
        interval = period_inverval_in_ms;
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
    private native void postCallIncoming();
    private native void postCallHungUp();
}

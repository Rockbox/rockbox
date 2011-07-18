/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Thomas Martitz
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

package org.rockbox.monitors;

import java.util.Timer;
import java.util.TimerTask;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

public class BatteryMonitor extends BroadcastReceiver
{
    private final IntentFilter mBattFilter;
    private final Context mContext;
    @SuppressWarnings("unused")
    private int mBattLevel; /* read by native code */
    
    /*
     * We get literally spammed with battery status updates
     * Therefore we actually unregister after each onReceive() and
     * setup a timer to re-register in 30s */
    public BatteryMonitor(Context c)
    {
        mBattFilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        mContext = c;
        Timer t = new Timer();
        TimerTask task = new TimerTask()
        {
            public void run()
            {
                attach();
            }
        };
        t.schedule(task, 5000, 30000);
        attach();
    }

    @Override
    public void onReceive(Context arg0, Intent intent)
    {
       int rawlevel = intent.getIntExtra("level", -1);
       int scale = intent.getIntExtra("scale", -1);
       if (rawlevel >= 0 && scale > 0)
           mBattLevel = (rawlevel * 100) / scale;
       else
           mBattLevel = -1;
       mContext.unregisterReceiver(this);
    }
    
    void attach()
    {
        mContext.registerReceiver(this, mBattFilter);
    }
}

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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

public class HeadphoneMonitor extends BroadcastReceiver
{
    public HeadphoneMonitor(Context c)
    {
        IntentFilter hpFilter = new IntentFilter(Intent.ACTION_HEADSET_PLUG);
        /* caution: hidden API; might break */
        IntentFilter noisyFilter = new IntentFilter("android.media.AUDIO_BECOMING_NOISY");
        
        c.registerReceiver(this, hpFilter);
        c.registerReceiver(new NoisyMonitor(), noisyFilter);
    }

    @Override
    public void onReceive(Context arg0, Intent intent)
    {
        int state = intent.getIntExtra("state", -1);
        postHpStateChanged(state);
    }

    /* audio becoming noise acts as headphones extracted */
    private class NoisyMonitor extends BroadcastReceiver
    {
        @Override
        public void onReceive(Context arg0, Intent arg1)
        {
            postHpStateChanged(0);   
        }
    }
    
    private synchronized native void postHpStateChanged(int state);
}

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

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;

public class RockboxActivity extends Activity 
{
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) 
    {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN
                       ,WindowManager.LayoutParams.FLAG_FULLSCREEN);
        final Intent intent = new Intent(this, 
                RockboxService.class);
        startService(intent);
        /* Now it gets a bit tricky:
         * The service is started in the same thread as we are now,
         * but the service also initializes the framebuffer
         * Unforunately, this happens *after* any of the default 
         * startup methods of an activity, so we need to poll for it 
         * 
         * In order to get the fb, we need to let the Service start up
         * run, we can wait in a separate thread for fb to get ready
         * This thread waits for the fb to become ready */
        new Thread(new Runnable()
        {
            public void run() 
            {
                try {
                    while (RockboxService.fb == null)
                        Thread.sleep(250);
                } catch (InterruptedException e) {
                } catch (Exception e) {
                    LOG(e.toString());
                }
                /* drawing needs to happen in ui thread */
                runOnUiThread(new Runnable() 
                {    @Override
                    public void run() {
                        setContentView(RockboxService.fb);
                        RockboxService.fb.invalidate();
                    }
                });
            }
        }).start();
    }
    
    public void onResume()
    {
        super.onResume();
        
        if (RockboxService.fb != null)
        {
            try {
                setContentView(RockboxService.fb);
            } catch (IllegalStateException e) {
                /* we are already using the View,
                 * need to remove it and re-attach it */
                ViewGroup g = (ViewGroup)RockboxService.fb.getParent();
                g.removeView(RockboxService.fb);
                setContentView(RockboxService.fb);
            }
            RockboxService.fb.resume();
        }
    }
    
    /* this is also called when the backlight goes off,
     * which is nice 
     */
    @Override
    protected void onPause() 
    {
        super.onPause();
        RockboxService.fb.suspend();
    }
    
    @Override
    protected void onStop() 
    {
        super.onStop();
        RockboxService.fb.suspend();
    }
    
    @Override
    protected void onDestroy() 
    {
        super.onDestroy();
        RockboxService.fb.suspend();
    }

    private void LOG(CharSequence text)
    {
        Log.d("Rockbox", (String) text);
    }
}
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
import android.app.ProgressDialog;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;

public class RockboxActivity extends Activity 
{
    private ProgressDialog loadingdialog;
    private RockboxService rbservice;
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) 
    {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN
                       ,WindowManager.LayoutParams.FLAG_FULLSCREEN);
        final Activity thisActivity = this;
        final Intent intent = new Intent(this, RockboxService.class);
        /* prepare a please wait dialog in case we need
         * to wait for unzipping libmisc.so
         */
        loadingdialog = new ProgressDialog(this);
        loadingdialog.setMessage("Rockbox is loading. Please wait...");
        loadingdialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        loadingdialog.setCancelable(false);
        startService(intent);
        rbservice = RockboxService.get_instance();
        /* Now it gets a bit tricky:
         * The service is started in the same thread as we are now,
         * but the service also initializes the framebuffer
         * Unfortunately, this happens *after* any of the default
         * startup methods of an activity, so we need to poll for it 
         * 
         * In order to get the fb, we need to let the Service start up
         * run, we can wait in a separate thread for fb to get ready
         * This thread waits for the fb to become ready */
        new Thread(new Runnable()
        {
            public void run() 
            {
                int i = 0;
                try {
                    while (true)
                    {
                        Thread.sleep(250);
                        if (isRockboxRunning())
                        	break;
                        /* if it's still null show the please wait dialog 
                         * but not before 0.5s are over */
                        if (!loadingdialog.isShowing() && i > 0)
                        {
                            runOnUiThread(new Runnable()
                            {
                                public void run()
                                {
                                    loadingdialog.show(); 
                                }
                            });                           
                        }
                        else 
                            i++ ;
                    }
                } catch (InterruptedException e) {
                }
                /* drawing needs to happen in ui thread */
                runOnUiThread(new Runnable() 
                {
                    public void run() {
                		loadingdialog.dismiss();
                		if (rbservice.get_fb() == null)
                		    throw new IllegalStateException("FB NULL");
                        rbservice.set_activity(thisActivity);
                		setContentView(rbservice.get_fb());
                        rbservice.get_fb().invalidate();
                    }
                });
            }
        }).start();
    }
    private boolean isRockboxRunning()
    {
        if (rbservice == null)
        	rbservice = RockboxService.get_instance();
        return (rbservice!= null && rbservice.isRockboxRunning() == true);    	
    }
    
    public void onResume()
    {
    	
        super.onResume();
        if (isRockboxRunning())
        {
            try {
                setContentView(rbservice.get_fb());
            } catch (IllegalStateException e) {
                /* we are already using the View,
                 * need to remove it and re-attach it */
                ViewGroup g = (ViewGroup)rbservice.get_fb().getParent();
                g.removeView(rbservice.get_fb());
                setContentView(rbservice.get_fb());
            } finally {
            	rbservice.set_activity(this);
                rbservice.get_fb().resume();
            }
        }
    }
    
    /* this is also called when the backlight goes off,
     * which is nice 
     */
    @Override
    protected void onPause() 
    {
        super.onPause();
        rbservice.set_activity(null);
        rbservice.get_fb().suspend();
    }
    
    @Override
    protected void onStop() 
    {
        super.onStop();
        rbservice.set_activity(null);
        rbservice.get_fb().suspend();
    }
    
    @Override
    protected void onDestroy() 
    {
        super.onDestroy();
        rbservice.set_activity(null);
        rbservice.get_fb().suspend();
    }
    
    private HostCallback hostcallback = null;
    public void waitForActivity(Intent i, HostCallback callback)
    {
    	if (hostcallback !=  null)
    	{
    		LOG("Something has gone wrong");
        }
        hostcallback = callback;
        startActivityForResult(i, 0);
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        hostcallback.onComplete(resultCode, data);
        hostcallback = null;
    }

    private void LOG(CharSequence text)
    {
        Log.d("Rockbox", (String) text);
    }
}

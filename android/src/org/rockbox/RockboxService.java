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

import org.rockbox.Helper.RunForegroundManager;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Timer;
import java.util.TimerTask;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import android.app.Activity;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.util.Log;

/* This class is used as the main glue between java and c.
 * All access should be done through RockboxService.get_instance() for safety.
 */

public class RockboxService extends Service 
{
    /* this Service is really a singleton class - well almost.
     * To do it properly this line should be instance = new RockboxService()
     * but apparently that doesnt work with the way android Services are created.
     */
    private static RockboxService instance = null;
    
    /* locals needed for the c code and rockbox state */
    private RockboxFramebuffer fb = null;
    private boolean mRockboxRunning = false;
    private Activity current_activity = null;
    private IntentFilter itf;
    private BroadcastReceiver batt_monitor;
    private RunForegroundManager fg_runner;
    @SuppressWarnings("unused")
    private int battery_level;

    @Override
    public void onCreate()
    {
   		instance = this;
        startservice();
    }
    
    public static RockboxService get_instance()
    {
    	return instance;
    }
    
    public RockboxFramebuffer get_fb()
    {
    	return fb;
    }
    /* framebuffer is initialised by the native code(!) so this is needed */
    public void set_fb(RockboxFramebuffer newfb)
    {
    	fb = newfb;
        mRockboxRunning = true;
    }
    
    public Activity get_activity()
    {
    	return current_activity;
    }
    public void set_activity(Activity a)
    {
    	current_activity = a;
    }
    

    private void do_start(Intent intent)
    {
        LOG("Start Service");
        
        /* Display a notification about us starting.  
         * We put an icon in the status bar. */
        try {
            fg_runner = new RunForegroundManager(this);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void LOG(CharSequence text)
    {
        Log.d("Rockbox", (String) text);
    }
    
    private void LOG(CharSequence text, Throwable tr)
    {
        Log.d("Rockbox", (String) text, tr);
    }

    public void onStart(Intent intent, int startId) {
        do_start(intent);
    }

    public int onStartCommand(Intent intent, int flags, int startId)
    {
        do_start(intent);
        return 1; /* old API compatibility: 1 == START_STICKY */
    }

    private void startservice() 
    {
        final int BUFFER = 8*1024;
        Thread rb = new Thread(new Runnable()
        {
            public void run()
            {
                LOG("main");
		        /* the following block unzips libmisc.so, which contains the files 
		         * we ship, such as themes. It's needed to put it into a .so file
		         * because there's no other way to ship files and have access
		         * to them from native code
		         */
		        try
		        {
		           BufferedOutputStream dest = null;
		           BufferedInputStream is = null;
		           ZipEntry entry;
		           File file = new File("/data/data/org.rockbox/" +
		           		"lib/libmisc.so");
		           /* use arbitrary file to determine whether extracting is needed */
		           File file2 = new File("/data/data/org.rockbox/" +
		           		"app_rockbox/rockbox/codecs/mpa.codec");
		           if (!file2.exists() || (file.lastModified() > file2.lastModified()))
		           {
		               ZipFile zipfile = new ZipFile(file);
		               Enumeration<? extends ZipEntry> e = zipfile.entries();
		               File folder;
		               while(e.hasMoreElements()) 
		               {
		                  entry = (ZipEntry) e.nextElement();
		                  LOG("Extracting: " +entry);
		                  if (entry.isDirectory())
		                  {
		                      folder = new File(entry.getName());
		                      LOG("mkdir "+ entry);
		                      try {
		                          folder.mkdirs();
		                      } catch (SecurityException ex) {
		                          LOG(ex.getMessage());
		                      }
		                      continue;
		                  }
		                  is = new BufferedInputStream(zipfile.getInputStream(entry),
		                          BUFFER);
		                  int count;
		                  byte data[] = new byte[BUFFER];
		                  folder = new File(new File(entry.getName()).getParent());
		                  LOG("" + folder.getAbsolutePath());
		                  if (!folder.exists())
		                      folder.mkdirs();
		                  FileOutputStream fos = new FileOutputStream(entry.getName());
		                  dest = new BufferedOutputStream(fos, BUFFER);
		                  while ((count = is.read(data, 0, BUFFER)) != -1)
		                     dest.write(data, 0, count);
		                  dest.flush();
		                  dest.close();
		                  is.close();
		               }
		           }
		        } catch(FileNotFoundException e) {
		            LOG("FileNotFoundException when unzipping", e);
		            e.printStackTrace();
		        } catch(IOException e) {
		            LOG("IOException when unzipping", e);
		            e.printStackTrace();
		        }
		
		        System.loadLibrary("rockbox");
                main();
            }
        },"Rockbox thread");
        rb.setDaemon(false);
        rb.start();
    }
    private native void main();
    
    /* returns true once rockbox is up and running.
     * This is considered done once the framebuffer is initialised
     */
    public boolean isRockboxRunning()
    {
    	return mRockboxRunning;
    }

    @Override
    public IBinder onBind(Intent intent) 
    {
        // TODO Auto-generated method stub
        return null;
    }

    
    @SuppressWarnings("unused")
    /*
     * Sets up the battery monitor which receives the battery level
     * about each 30 seconds
     */
    private void initBatteryMonitor()
    {
        itf = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        batt_monitor = new BroadcastReceiver() 
        {            
            @Override
            public void onReceive(Context context, Intent intent) 
            {
                /* we get literally spammed with battery statuses 
                 * if we don't delay the re-attaching
                 */
                TimerTask tk = new TimerTask() 
                {
                    public void run() 
                    {
                        registerReceiver(batt_monitor, itf);
                    }
                };
                Timer t = new Timer();
                context.unregisterReceiver(this);
                int rawlevel = intent.getIntExtra("level", -1);
                int scale = intent.getIntExtra("scale", -1);
                if (rawlevel >= 0 && scale > 0)
                    battery_level = (rawlevel * 100) / scale;
                else
                    battery_level = -1;
                /* query every 30s should be sufficient */ 
                t.schedule(tk, 30000);
            }
        };
        registerReceiver(batt_monitor, itf);
    }
    
    public void startForeground()
    {
        fg_runner.startForeground();
    }
    
    public void stopForeground()
    {
        fg_runner.stopForeground();
    }

    @Override
    public void onDestroy() 
    {
        super.onDestroy();
        fb.destroy();
        /* Make sure our notification is gone. */
        stopForeground();
    }
}

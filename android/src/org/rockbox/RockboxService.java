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

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Enumeration;
import java.util.Timer;
import java.util.TimerTask;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import android.util.Log;

public class RockboxService extends Service 
{
    /* this Service is really a singleton class */
    public static RockboxFramebuffer fb = null;
    private static RockboxService instance;
    private Notification notification;
    private static final Class<?>[] mStartForegroundSignature = 
        new Class[] { int.class, Notification.class };
    private static final Class<?>[] mStopForegroundSignature = 
        new Class[] { boolean.class };

    private NotificationManager mNM;
    private Method mStartForeground;
    private Method mStopForeground;
    private Object[] mStartForegroundArgs = new Object[2];
    private Object[] mStopForegroundArgs = new Object[1];
    private IntentFilter itf;
    private BroadcastReceiver batt_monitor;
    @SuppressWarnings("unused")
    private int battery_level;

    @Override
    public void onCreate()
    {
        mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
        try 
        {
            mStartForeground = getClass().getMethod("startForeground",
                    mStartForegroundSignature);
            mStopForeground = getClass().getMethod("stopForeground",
                    mStopForegroundSignature);
        } 
        catch (NoSuchMethodException e) 
        {
            /* Running on an older platform: fall back to old API */
            mStartForeground = mStopForeground = null;
        }
        instance = this;
        startservice();
    }

    private void do_start(Intent intent)
    {
        LOG("Start Service");
        
        /* Display a notification about us starting.  
         * We put an icon in the status bar. */
        create_notification();
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
    
    /* all below is heavily based on the examples found on
     * http://developer.android.com/reference/android/app/Service.html
     */
    
    private void create_notification()
    {
        /* For now we'll use the same text for the ticker and the 
         * expanded notification */
        CharSequence text = getText(R.string.notification);
        /* Set the icon, scrolling text and timestamp */
        notification = new Notification(R.drawable.icon, text,
                System.currentTimeMillis());

        /* The PendingIntent to launch our activity if the user selects
         * this notification */
        Intent intent = new Intent(this, RockboxActivity.class);
        PendingIntent contentIntent = 
                PendingIntent.getActivity(this, 0, intent, 0);

        /*  Set the info for the views that show in the notification panel. */
        notification.setLatestEventInfo(this, 
                getText(R.string.notification), text, contentIntent);
    }

    public static void startForeground() 
    {
        if (instance != null) 
        {
            /* 
             * Send the notification.
             * We use a layout id because it is a unique number.  
             * We use it later to cancel.
             */
            instance.mNM.notify(R.string.notification, instance.notification);
            /*
             * this call makes the service run as foreground, which
             * provides enough cpu time to do music decoding in the 
             * background
             */
            instance.startForegroundCompat(R.string.notification, 
                    instance.notification);
        }
    }
    
    public static void stopForeground() 
    {
        if (instance.notification != null)
        {
            instance.stopForegroundCompat(R.string.notification);
            instance.mNM.cancel(R.string.notification);
        }
    }

    /**
     * This is a wrapper around the new startForeground method, using the older
     * APIs if it is not available.
     */
    void startForegroundCompat(int id, Notification notification) 
    {
        if (mStartForeground != null) {
            mStartForegroundArgs[0] = Integer.valueOf(id);
            mStartForegroundArgs[1] = notification;
            try {
                mStartForeground.invoke(this, mStartForegroundArgs);
            } catch (InvocationTargetException e) {
                /* Should not happen. */
                LOG("Unable to invoke startForeground", e);
            } catch (IllegalAccessException e) {
                /* Should not happen. */
                LOG("Unable to invoke startForeground", e);
            }
            return;
        }

        /* Fall back on the old API.*/
        setForeground(true);
        mNM.notify(id, notification);
    }

    /**
     * This is a wrapper around the new stopForeground method, using the older
     * APIs if it is not available.
     */
    void stopForegroundCompat(int id) 
    {
        if (mStopForeground != null) {
            mStopForegroundArgs[0] = Boolean.TRUE;
            try {
                mStopForeground.invoke(this, mStopForegroundArgs);
            } catch (InvocationTargetException e) {
                /* Should not happen. */
                LOG("Unable to invoke stopForeground", e);
            } catch (IllegalAccessException e) {
                /* Should not happen. */
                LOG("Unable to invoke stopForeground", e);
            }
            return;
        }

        /* Fall back on the old API.  Note to cancel BEFORE changing the
         * foreground state, since we could be killed at that point. */
        mNM.cancel(id);
        setForeground(false);
    }

    @Override
    public void onDestroy() 
    {
        super.onDestroy();
        /* Make sure our notification is gone. */
        stopForegroundCompat(R.string.notification);
    }
}

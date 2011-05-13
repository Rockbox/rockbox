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
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.util.Enumeration;
import java.util.Timer;
import java.util.TimerTask;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.rockbox.Helper.MediaButtonReceiver;
import org.rockbox.Helper.RunForegroundManager;

import android.app.Activity;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.os.ResultReceiver;
import android.util.Log;
import android.view.KeyEvent;

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
    private static volatile boolean rockbox_running;
    private Activity current_activity = null;
    private IntentFilter itf;
    private BroadcastReceiver batt_monitor;
    private RunForegroundManager fg_runner;
    private MediaButtonReceiver mMediaButtonReceiver;
    private int battery_level;
    private ResultReceiver resultReceiver;

    public static final int RESULT_INVOKING_MAIN = 0;
    public static final int RESULT_LIB_LOAD_PROGRESS = 1;
    public static final int RESULT_SERVICE_RUNNING = 3;
    public static final int RESULT_ERROR_OCCURED = 4;
    public static final int RESULT_LIB_LOADED = 5;
    public static final int RESULT_ROCKBOX_EXIT = 6;

    @Override
    public void onCreate()
    {
        instance = this;
        mMediaButtonReceiver = new MediaButtonReceiver(this);
        fg_runner = new RunForegroundManager(this);
    }

    public static RockboxService get_instance()
    {
        /* don't call the construtor here, the instances are managed by
         * android, so we can't just create a new one */
        return instance;
    }

    public boolean isRockboxRunning()
    {
        return rockbox_running;
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
        LOG("Start RockboxService (Intent: " + intent.getAction() + ")");

        if (intent.getAction().equals("org.rockbox.ResendTrackUpdateInfo"))
        {
            if (rockbox_running)
                fg_runner.resendUpdateNotification();
            return;
        }

        if (intent.hasExtra("callback"))
            resultReceiver = (ResultReceiver) intent.getParcelableExtra("callback");

        if (!rockbox_running)
            startservice();
        if (resultReceiver != null)
            resultReceiver.send(RESULT_LIB_LOADED, null);

        if (intent.getAction().equals(Intent.ACTION_MEDIA_BUTTON))
        {
            /* give it a bit of time so we can register button presses 
             * sleeping longer doesn't work here, apparently Android 
             * surpresses long sleeps during intent handling */
            try {
                Thread.sleep(50);
            } catch (InterruptedException e) { }

            KeyEvent kev = intent.getParcelableExtra(Intent.EXTRA_KEY_EVENT);
            RockboxFramebuffer.buttonHandler(kev.getKeyCode(), 
                                kev.getAction() == KeyEvent.ACTION_DOWN);
        }

        /* (Re-)attach the media button receiver, in case it has been lost */
        mMediaButtonReceiver.register();
        if (resultReceiver != null)
            resultReceiver.send(RESULT_SERVICE_RUNNING, null);

        rockbox_running = true;
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
        final Object lock = new Object();
        Thread rb = new Thread(new Runnable()
        {
            public void run()
            {
                final int BUFFER = 8*1024;
                String rockboxDirPath = "/data/data/org.rockbox/app_rockbox/rockbox";
                File rockboxDir = new File(rockboxDirPath);

                /* load library before unzipping which may take a while */
                synchronized (lock) {
                    System.loadLibrary("rockbox");
                    lock.notify();
                }

                /* the following block unzips libmisc.so, which contains the files 
                 * we ship, such as themes. It's needed to put it into a .so file
                 * because there's no other way to ship files and have access
                 * to them from native code
                 */
                File libMisc = new File("/data/data/org.rockbox/lib/libmisc.so");
                /* use arbitrary file to determine whether extracting is needed */
                File arbitraryFile = new File(rockboxDir, "viewers.config");
                if (!arbitraryFile.exists() || (libMisc.lastModified() > arbitraryFile.lastModified()))
                {
                    try
                    {
                        Bundle progressData = new Bundle();
                        byte data[] = new byte[BUFFER];
                        ZipFile zipfile = new ZipFile(libMisc);
                        Enumeration<? extends ZipEntry> e = zipfile.entries();
                        progressData.putInt("max", zipfile.size());

                        while(e.hasMoreElements())
                        {
                           ZipEntry entry = (ZipEntry) e.nextElement();
                           File file;
                           /* strip off /.rockbox when extracting */
                           String fileName = entry.getName();
                           int slashIndex = fileName.indexOf('/', 1);
                           file = new File(rockboxDirPath + fileName.substring(slashIndex));

                           if (!entry.isDirectory())
                           {
                               /* Create the parent folders if necessary */
                               File folder = new File(file.getParent());
                               if (!folder.exists())
                                   folder.mkdirs();

                               /* Extract file */
                               BufferedInputStream is = new BufferedInputStream(zipfile.getInputStream(entry), BUFFER);
                               FileOutputStream fos = new FileOutputStream(file);
                               BufferedOutputStream dest = new BufferedOutputStream(fos, BUFFER);

                               int count;
                               while ((count = is.read(data, 0, BUFFER)) != -1)
                                  dest.write(data, 0, count);

                               dest.flush();
                               dest.close();
                               is.close();
                           }

                           if (resultReceiver != null) {
                               progressData.putInt("value", progressData.getInt("value", 0) + 1);
                               resultReceiver.send(RESULT_LIB_LOAD_PROGRESS, progressData);
                           }
                        }
                    } catch(Exception e) {
                        LOG("Exception when unzipping", e);
                        e.printStackTrace();
                        if (resultReceiver != null) {
                            Bundle bundle = new Bundle();
                            bundle.putString("error", getString(R.string.error_extraction));
                            resultReceiver.send(RESULT_ERROR_OCCURED, bundle);
                        }
                    }
                }

                /* Generate default config if none exists yet */
                File rockboxConfig = new File(Environment.getExternalStorageDirectory(), "rockbox/config.cfg");
                if (!rockboxConfig.exists()) {
                    File rbDir = new File(rockboxConfig.getParent());
                    if (!rbDir.exists())
                        rbDir.mkdirs();

                    OutputStreamWriter strm;
                    try {
                        strm = new OutputStreamWriter(new FileOutputStream(rockboxConfig));
                        strm.write("# config generated by RockboxService\n");
                        strm.write("start directory: " + Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + "\n");
                        strm.write("lang: /.rockbox/langs/" + getString(R.string.rockbox_language_file) + "\n");
                        strm.close();
                    } catch(Exception e) {
                        LOG("Exception when writing default config", e);
                    }
                }

                /* Start native code */
                if (resultReceiver != null)
                    resultReceiver.send(RESULT_INVOKING_MAIN, null);

                main();

                if (resultReceiver != null)
                    resultReceiver.send(RESULT_ROCKBOX_EXIT, null);

                LOG("Stop service: main() returned");
                stopSelf(); /* serivce is of no use anymore */
            }
        }, "Rockbox thread");
        rb.setDaemon(false);
        /* wait at least until the library is loaded */
        synchronized (lock)
        {
            rb.start();
            while(true)
            {
                try {
                    lock.wait();
                } catch (InterruptedException e) {
                    continue;
                }
                break;
            }
        }
    }

    private native void main();

    @Override
    public IBinder onBind(Intent intent)
    {
        // TODO Auto-generated method stub
        return null;
    }


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

    void startForeground()
    {
        fg_runner.startForeground();
    }

    void stopForeground()
    {
        fg_runner.stopForeground();
    }

    @Override
    public void onDestroy()
    {
        super.onDestroy();
        /* Don't unregister so we can receive them (and startup the service)
         * after idle poweroff. Hopefully it's ok if mMediaButtonReceiver is
         * garbage collected.
         *  mMediaButtonReceiver.unregister(); */
        mMediaButtonReceiver = null;
        /* Make sure our notification is gone. */
        stopForeground();
        instance = null;
        rockbox_running = false;
        System.runFinalization();
        /* exit() seems unclean but is needed in order to get the .so file garbage 
         * collected, otherwise Android caches this Service and librockbox.so
         * The library must be reloaded to zero the bss and reset data
         * segment */
        System.exit(0);
    }
}

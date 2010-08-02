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
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import android.app.Activity;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;

public class RockboxActivity extends Activity {
    /** Called when the activity is first created. */
    public RockboxFramebuffer fb;
    private Thread rb;
    static final int BUFFER = 2048;
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    	LOG("start rb");
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN
                       ,WindowManager.LayoutParams.FLAG_FULLSCREEN); 
        fb = new RockboxFramebuffer(this);
        if (true) {
        try 
        {
           BufferedOutputStream dest = null;
           BufferedInputStream is = null;
           ZipEntry entry;
           File file = new File("/data/data/org.rockbox/lib/libmisc.so");
           /* use arbitary file to determine whether extracting is needed */
           File file2 = new File("/data/data/org.rockbox/app_rockbox/rockbox/codecs/mpa.codec");
           if (!file2.exists() || (file.lastModified() > file2.lastModified()))
           {
	           ZipFile zipfile = new ZipFile(file);
	           Enumeration<? extends ZipEntry> e = zipfile.entries();
	           File folder;
	           while(e.hasMoreElements()) {
	              entry = (ZipEntry) e.nextElement();
	              LOG("Extracting: " +entry);
	              if (entry.isDirectory())
	              {
	            	  folder = new File(entry.getName());
	            	  LOG("mkdir "+ entry);
	            	  try {
	            		  folder.mkdirs();
	            	  } catch (SecurityException ex){
	            		  LOG(ex.getMessage());
	            	  }
	            	  continue;
	              }
	              is = new BufferedInputStream(zipfile.getInputStream(entry));
	              int count;
	              byte data[] = new byte[BUFFER];
	              folder = new File(new File(entry.getName()).getParent());
	              LOG("" + folder.getAbsolutePath());
	              if (!folder.exists())
	            	  folder.mkdirs();
	              FileOutputStream fos = new FileOutputStream(entry.getName());
	              dest = new BufferedOutputStream(fos, BUFFER);
	              while ((count = is.read(data, 0, BUFFER)) != -1) {
	                 dest.write(data, 0, count);
	              }
	              dest.flush();
	              dest.close();
	              is.close();
	           }
           }
        } catch(Exception e) {
           e.printStackTrace();
        }}
        Rect r = new Rect();
        fb.getDrawingRect(r);
        LOG(r.left + " " + r.top + " " + r.right + " " + r.bottom);
    	rb = new Thread(new Runnable()
    	{
    		public void run()
    		{
    			main();
    		}
    	},"Rockbox thread");
        System.loadLibrary("rockbox");
        rb.setDaemon(false);
        setContentView(fb);
    }

	private void LOG(CharSequence text)
	{
		Log.d("RockboxBootloader", (String) text);
	}

    public synchronized void onStart()
    {
    	super.onStart();
    	if (!rb.isAlive())
    		rb.start();
    }
    
    public void onPause()
    {
    	super.onPause();
    }
    
    public void onResume()
    {
    	super.onResume();
    	switch (rb.getState()) {
    	case BLOCKED: LOG("BLOCKED"); break;
    	case RUNNABLE: LOG("RUNNABLE"); break;
    	case NEW: LOG("NEW"); break;
    	case TERMINATED: LOG("TERMINATED"); break;
    	case TIMED_WAITING: LOG("TIMED_WAITING"); break;
    	case WAITING: LOG("WAITING"); break;
    	}
    }


    private native void main();
}
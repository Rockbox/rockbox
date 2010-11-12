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
import android.os.Handler;
import android.os.ResultReceiver;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;

public class RockboxActivity extends Activity 
{
    private RockboxService rbservice;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) 
    {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                             WindowManager.LayoutParams.FLAG_FULLSCREEN);

        Intent intent = new Intent(this, RockboxService.class);
        intent.putExtra("callback", new ResultReceiver(new Handler(getMainLooper())) {
            private ProgressDialog loadingdialog;

            private void createProgressDialog()
            {
                loadingdialog = new ProgressDialog(RockboxActivity.this);
                loadingdialog.setMessage(getString(R.string.rockbox_extracting));
                loadingdialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
                loadingdialog.setIndeterminate(true);
                loadingdialog.setCancelable(false);
                loadingdialog.show();
            }

            @Override
            protected void onReceiveResult(final int resultCode, final Bundle resultData)
            {
                switch (resultCode) {
                    case RockboxService.RESULT_LIB_LOADED:
                        rbservice = RockboxService.get_instance();
                        if (loadingdialog != null)
                            loadingdialog.setIndeterminate(true);
                        break;
                    case RockboxService.RESULT_LIB_LOAD_PROGRESS:
                        if (loadingdialog == null)
                            createProgressDialog();

                        loadingdialog.setIndeterminate(false);
                        loadingdialog.setMax(resultData.getInt("max", 100));
                        loadingdialog.setProgress(resultData.getInt("value", 0));
                        break;
                    case RockboxService.RESULT_FB_INITIALIZED:
                        attachFramebuffer();
                        if (loadingdialog != null)
                            loadingdialog.dismiss();
                        break;
                    case RockboxService.RESULT_ERROR_OCCURED:
                        Toast.makeText(RockboxActivity.this, resultData.getString("error"), Toast.LENGTH_LONG);
                        break;
                }
            }
        });
        startService(intent);
    }

    private boolean isRockboxRunning()
    {
        if (rbservice == null)
            rbservice = RockboxService.get_instance();
        return (rbservice!= null && rbservice.isRockboxRunning() == true);    	
    }

    private void attachFramebuffer()
    {
        View rbFramebuffer = rbservice.get_fb();
        try {
            setContentView(rbFramebuffer);
        } catch (IllegalStateException e) {
            /* we are already using the View,
             * need to remove it and re-attach it */
            ViewGroup g = (ViewGroup) rbFramebuffer.getParent();
            g.removeView(rbFramebuffer);
            setContentView(rbFramebuffer);
        } finally {
            rbFramebuffer.requestFocus();
            rbservice.set_activity(this);
        }
    }

    public void onResume()
    {
        super.onResume();
        if (isRockboxRunning())
            attachFramebuffer();
    }
    
    /* this is also called when the backlight goes off,
     * which is nice 
     */
    @Override
    protected void onPause() 
    {
        super.onPause();
        if (rbservice != null)
        {
        	rbservice.set_activity(null);
        	rbservice.get_fb().dispatchWindowVisibilityChanged(View.INVISIBLE);
        }
    }
    
    @Override
    protected void onStop() 
    {
        super.onStop();
        if (rbservice != null)
        	rbservice.set_activity(null);
    }
    
    @Override
    protected void onDestroy() 
    {
        super.onDestroy();
        if (rbservice != null)
        	rbservice.set_activity(null);
    }

    private void LOG(CharSequence text)
    {
        Log.d("Rockbox", (String) text);
    }
}

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

import java.nio.ByteBuffer;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewConfiguration;

public class RockboxFramebuffer extends SurfaceView 
                                 implements SurfaceHolder.Callback
{
    private final DisplayMetrics metrics;
    private final ViewConfiguration view_config;
    private ByteBuffer native_buf;
    private Bitmap btm;

    /* first stage init; needs to run from a thread that has a Looper 
     * setup stuff that needs a Context */
    public RockboxFramebuffer(Context c)
    {
        super(c);
         
        metrics = c.getResources().getDisplayMetrics();
        view_config = ViewConfiguration.get(c);
        getHolder().addCallback(this);
        /* Needed so we can catch KeyEvents */
        setFocusable(true);
        setFocusableInTouchMode(true);
        setClickable(true);
        /* don't draw until native is ready (2nd stage) */
        setEnabled(false);
    }

    /* second stage init; called from Rockbox with information about the 
     * display framebuffer */
    @SuppressWarnings("unused")
    private void java_lcd_init(int lcd_width, int lcd_height, ByteBuffer native_fb)
    {
        btm = Bitmap.createBitmap(lcd_width, lcd_height, Bitmap.Config.RGB_565);
        native_buf = native_fb;
        setEnabled(true);
    }

    @SuppressWarnings("unused")
    private void java_lcd_update()
    {
        SurfaceHolder holder = getHolder();                            
        Canvas c = holder.lockCanvas(null);
        btm.copyPixelsFromBuffer(native_buf);
        synchronized (holder)
        { /* draw */
            c.drawBitmap(btm, 0.0f, 0.0f, null);
        }
        holder.unlockCanvasAndPost(c);
    }
    
    @SuppressWarnings("unused")
    private void java_lcd_update_rect(int x, int y, int width, int height)
    {
        SurfaceHolder holder = getHolder();         
        Rect dirty = new Rect(x, y, x+width, y+height);
        Canvas c = holder.lockCanvas(dirty);
        /* can't copy a partial buffer,
         * but it doesn't make a noticeable difference anyway */
        btm.copyPixelsFromBuffer(native_buf);
        synchronized (holder)
        {   /* draw */
            c.drawBitmap(btm, dirty, dirty, null);   
        }
        holder.unlockCanvasAndPost(c);
    }

    @SuppressWarnings("unused")
    private void LOG(CharSequence text)
    {
        Log.d("Rockbox", (String) text);
    }

    public boolean onTouchEvent(MotionEvent me)
    {
        int x = (int) me.getX();
        int y = (int) me.getY();

        switch (me.getAction())
        {
        case MotionEvent.ACTION_CANCEL:
        case MotionEvent.ACTION_UP:
            touchHandler(false, x, y);
            return true;
        case MotionEvent.ACTION_MOVE:
        case MotionEvent.ACTION_DOWN:
            touchHandler(true, x, y);
            return true;
        }

        return false;
    }

    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        return buttonHandler(keyCode, true);
    }

    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        return buttonHandler(keyCode, false);
    }
 
    @SuppressWarnings("unused")
    private int getDpi()
    {
        return metrics.densityDpi;
    }
    
    @SuppressWarnings("unused")
    private int getScrollThreshold()
    {
        return view_config.getScaledTouchSlop();
    }

    private native void touchHandler(boolean down, int x, int y);
    public native static boolean buttonHandler(int keycode, boolean state);

    public native void surfaceCreated(SurfaceHolder holder);
    public native void surfaceDestroyed(SurfaceHolder holder);
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
    {
    }
}

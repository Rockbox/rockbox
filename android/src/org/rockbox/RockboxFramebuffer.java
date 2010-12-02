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

import org.rockbox.Helper.MediaButtonReceiver;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;

public class RockboxFramebuffer extends View
{
    private Bitmap btm;
    private Rect rect;
    private ByteBuffer native_buf;
    private MediaButtonReceiver media_monitor;
    private final DisplayMetrics metrics;
    private final ViewConfiguration view_config;

    public RockboxFramebuffer(Context c, int lcd_width, 
                              int lcd_height, ByteBuffer native_fb)
    {
        super(c);
        /* Needed so we can catch KeyEvents */
        setFocusable(true);
        setFocusableInTouchMode(true);
        setClickable(true);
        btm = Bitmap.createBitmap(lcd_width, lcd_height, Bitmap.Config.RGB_565);
        rect = new Rect();
        native_buf = native_fb;
        media_monitor = new MediaButtonReceiver(c);
        media_monitor.register();
        /* the service needs to know the about us */
        ((RockboxService)c).set_fb(this);
        
        metrics = c.getResources().getDisplayMetrics();
        view_config = ViewConfiguration.get(c);
    }

    public void onDraw(Canvas c) 
    {
        /* can't copy a partial buffer :( */
        btm.copyPixelsFromBuffer(native_buf);
        c.getClipBounds(rect);
        c.drawBitmap(btm, rect, rect, null);
        post_update_done();
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

    public void destroy()
    {
        set_lcd_active(0);
        media_monitor.unregister();
    }

    @Override
    protected void onWindowVisibilityChanged(int visibility)
    {
        super.onWindowVisibilityChanged(visibility);

        switch (visibility) {
            case VISIBLE:
                set_lcd_active(1);
                break;
            case GONE:
            case INVISIBLE:
                set_lcd_active(0);
                break;
        }
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

    private native void post_update_done();
    private native void set_lcd_active(int active);
    private native void touchHandler(boolean down, int x, int y);
    private native boolean buttonHandler(int keycode, boolean state);
}

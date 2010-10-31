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
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

public class RockboxFramebuffer extends View
{
    private Bitmap btm;
    private ByteBuffer native_buf;
    private MediaButtonReceiver media_monitor;

    public RockboxFramebuffer(Context c, int lcd_width, 
                              int lcd_height, ByteBuffer native_fb)
    {
        super(c);
        /* Needed so we can catch KeyEvents */
        setFocusable(true);
        setFocusableInTouchMode(true);
        setClickable(true);
        btm = Bitmap.createBitmap(lcd_width, lcd_height, Bitmap.Config.RGB_565);
        native_buf = native_fb;
        requestFocus();
        media_monitor = new MediaButtonReceiver(c);
        media_monitor.register();
        /* the service needs to know the about us */
        ((RockboxService)c).set_fb(this);
    }

    public void onDraw(Canvas c) 
    {
        c.drawBitmap(btm, 0.0f, 0.0f, null);
    }
    
    @SuppressWarnings("unused")
    private void java_lcd_update()
    {
        btm.copyPixelsFromBuffer(native_buf);
        postInvalidate();
    }
    
    @SuppressWarnings("unused")
    private void java_lcd_update_rect(int x, int y, int w, int h)
    {
        /* can't copy a partial buffer */
        btm.copyPixelsFromBuffer(native_buf);
        postInvalidate(x, y, x+w, y+h);
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

    /* the two below should only be called from the activity thread */
    public void suspend()
    {    /* suspend, Rockbox will not make any lcd updates */
        set_lcd_active(0);
    }
    public void resume()
    {    
        /* Needed so we can catch KeyEvents */
        setFocusable(true);
        setFocusableInTouchMode(true);
        setClickable(true);
        requestFocus();
        set_lcd_active(1);
    }

    public void destroy()
    {
        suspend();
        media_monitor.unregister();
    }

    public native void set_lcd_active(int active);
    public native void touchHandler(boolean down, int x, int y);
    public native boolean buttonHandler(int keycode, boolean state);
    
}

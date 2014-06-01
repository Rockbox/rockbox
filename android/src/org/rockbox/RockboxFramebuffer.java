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
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewConfiguration;
import android.graphics.PixelFormat;

public class RockboxFramebuffer extends SurfaceView
                                 implements SurfaceHolder.Callback
{
    private final DisplayMetrics metrics;
    private final ViewConfiguration view_config;
    private Bitmap btm;

    private int srcWidth=0,srcHeight=0;            /* static value from Rockbox internal framebuffer */
    private int desWidth, desHeight;               /* from the current running android device */
    private int paddingHeight=0,paddingWidth=0;    /* the size of black bar to keep scale aspect ratio*/
    private int fixedWidth, fixedHeight;           /* srcWidth+paddingWidth, srcHeight+paddingHeight */
    private float scaleWidthFactor,scaleHeightFactor;

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
        desWidth = metrics.widthPixels;
        desHeight = metrics.heightPixels;
    }

    private void initialize(int lcd_width, int lcd_height)
    {
        srcWidth = lcd_width;
        srcHeight = lcd_height;
        scaleWidthFactor = ((float)desWidth) / srcWidth;
        scaleHeightFactor = ((float)desHeight) / srcHeight;

        /* padding to keep screen aspect ratio*/
        if (scaleHeightFactor > scaleWidthFactor)
            paddingHeight = (int)Math.ceil(srcWidth * desHeight / desWidth) - srcHeight;

        if ( scaleWidthFactor > scaleHeightFactor)
            paddingWidth = (int)Math.ceil(srcHeight * desWidth / desHeight) - srcWidth;

        fixedWidth = srcWidth + paddingWidth;
        fixedHeight = srcHeight + paddingHeight;

        /* setFixedSize to use hardware scaler */
        getHolder().setFixedSize(fixedWidth,fixedHeight);
        getHolder().setFormat(new PixelFormat().RGB_565);

        btm = Bitmap.createBitmap(srcWidth, srcHeight, Bitmap.Config.RGB_565);

        setEnabled(true);
    }

    private void update(ByteBuffer framebuffer)
    {
        SurfaceHolder holder = getHolder();
        Canvas c;
        Rect dirty = new Rect();
        holder.setFixedSize(fixedWidth,fixedHeight);
        dirty.set(0,0,fixedWidth,fixedHeight);
        c = holder.lockCanvas(dirty);
        if (c == null)
            return;

        btm.copyPixelsFromBuffer(framebuffer);
        synchronized (holder)
        { /* draw */
            c.drawBitmap(btm, 0.0f, 0.0f, null);
        }
        holder.unlockCanvasAndPost(c);
    }

    private void update(ByteBuffer framebuffer, Rect dirty)
    {
        SurfaceHolder holder = getHolder();
        Canvas c;
        holder.setFixedSize(fixedWidth,fixedHeight);
        if(dirty.isEmpty()) /*  (left >= right or top >= bottom) */
            dirty.sort();
        c = holder.lockCanvas(dirty);
        if (c == null)
            return;

        /* can't copy a partial buffer, but it doesn't make a noticeable difference anyway */
        btm.copyPixelsFromBuffer(framebuffer);
        synchronized (holder)
        {   /* draw */
            c.drawBitmap(btm, dirty, dirty, null);
        }
        holder.unlockCanvasAndPost(c);
    }

    public boolean onTouchEvent(MotionEvent me)
    {
        int x = (int) me.getRawX();
        int y = (int) me.getRawY();
        /* convert */
        x =  (paddingWidth  > 0) ? (int)( x / scaleHeightFactor):(int)( x / scaleWidthFactor);
        y =  (paddingHeight > 0) ? (int)( y / scaleWidthFactor) :(int)( y / scaleHeightFactor);

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

    private int getDpi()
    {
        if (desHeight > srcHeight)
            return  (int)(metrics.densityDpi / scaleHeightFactor);
        return metrics.densityDpi;
    }


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

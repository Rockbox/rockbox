/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 Antoine Cellerier <dionoea at videolan dot org>
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

package org.rockbox.widgets;

import java.io.File;
import org.rockbox.R;
import org.rockbox.RockboxActivity;
import org.rockbox.RockboxService;
import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.appwidget.AppWidgetProviderInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.view.KeyEvent;
import android.view.View;
import android.widget.RemoteViews;

public class RockboxWidgetProvider extends AppWidgetProvider
{
    static RockboxWidgetProvider mInstance;
    public RockboxWidgetProvider()
    {
        super();
        mInstance = this;
    }

    @Override
    public void onUpdate(Context context, AppWidgetManager appWidgetManager, int[] appWidgetIds)
    {
        final int N = appWidgetIds.length;
        for (int i = 0; i < N; i++)
        {
            int appWidgetId = appWidgetIds[i];
            updateAppWidget(context, appWidgetManager, appWidgetId, null);

        }
    }
    
    public static RockboxWidgetProvider getInstance()
    {   
        /* no new instance here, instanced by android */
        return mInstance;
    }

    @Override
    public void onDeleted(Context context, int[] appWidgetIds)
    {
    }

    @Override
    public void onEnabled(Context context)
    {
    }

    @Override
    public void onDisabled(Context context)
    {
    }

    @Override
    public void onReceive(Context context, Intent intent)
    {
        String action = intent.getAction();
        if (action.equals("org.rockbox.TrackUpdateInfo") ||
            action.equals("org.rockbox.TrackFinish") ||
            action.equals("org.rockbox.UpdateState"))
        {
            AppWidgetManager gm = AppWidgetManager.getInstance(context);
            int[] appWidgetIds = gm.getAppWidgetIds(new ComponentName(context, this.getClass()));
            final int N = appWidgetIds.length;
            for (int i = 0; i < N; i++)
            {
                updateAppWidget(context, gm, appWidgetIds[i], intent);
            }
        }
        else
        {
            super.onReceive(context, intent);
        }
    }

     public void updateAppWidget(Context context, AppWidgetManager appWidgetManager, int appWidgetId, Intent args)
     {
        AppWidgetProviderInfo provider = appWidgetManager.getAppWidgetInfo(appWidgetId);
        RemoteViews views = null;
        views = new RemoteViews(context.getPackageName(), provider.initialLayout);

        Intent intent = new Intent(context, RockboxActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, intent, 0);
        views.setOnClickPendingIntent(R.id.infoDisplay, pendingIntent);
        views.setOnClickPendingIntent(R.id.logo, pendingIntent);

        RockboxWidgetConfigure.WidgetPref state = RockboxWidgetConfigure.loadWidgetPref(context, appWidgetId);

        /* enable/disable PREV */
        if (state.enablePrev)
        {
            views.setOnClickPendingIntent(R.id.prev, 
                                          newPendingIntent(context,
                                            KeyEvent.KEYCODE_MEDIA_PREVIOUS));
        }
        else
            views.setViewVisibility(R.id.prev, View.GONE);

        /* enable/disable PLAY/PAUSE */
        if (state.enablePlayPause)
        {
            views.setOnClickPendingIntent(R.id.playPause, 
                                          newPendingIntent(context,
                                            KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE));
        }
        else
            views.setViewVisibility(R.id.playPause, View.GONE);

        /* enable/disable NEXT */
        if (state.enableNext)
        {
            views.setOnClickPendingIntent(R.id.next, 
                                          newPendingIntent(context,
                                            KeyEvent.KEYCODE_MEDIA_NEXT));
        }
        else
            views.setViewVisibility(R.id.next, View.GONE);

        /* enable/disable STOP */
        if (state.enableStop)
        {
            views.setOnClickPendingIntent(R.id.stop, 
                                          newPendingIntent(context,
                                            KeyEvent.KEYCODE_MEDIA_STOP));
        }
        else
            views.setViewVisibility(R.id.stop, View.GONE);

        if (!state.enableAA)
            views.setViewVisibility(R.id.logo, View.GONE);

        if (args != null)
        {
            if (args.getAction().equals("org.rockbox.TrackUpdateInfo"))
            {
                CharSequence title = args.getCharSequenceExtra("title");
                CharSequence artist = args.getCharSequenceExtra("artist");
                CharSequence album = args.getCharSequenceExtra("album");
                views.setTextViewText(R.id.infoDisplay, title+"\n"+artist+"\n"+album);
                CharSequence albumart = args.getCharSequenceExtra("albumart");
                /* Uri.fromFile() is buggy in <2.2
                 * http://stackoverflow.com/questions/3004713/get-content-uri-from-file-path-in-android */
                if (albumart != null)
                    views.setImageViewUri(R.id.logo, Uri.parse(
                                    new File(albumart.toString()).toString()));
                else
                    views.setImageViewResource(R.id.logo, R.drawable.rockbox);
            }
            else if (args.getAction().equals("org.rockbox.TrackFinish"))
            {
                // FIXME: looks like this event is always fired earlier than
                // the actual track change (a few seconds)
                views.setTextViewText(R.id.infoDisplay, context.getString(R.string.appwidget_infoDisplay));
                views.setImageViewResource(R.id.logo, R.drawable.rockbox);
            }
            else if (args.getAction().equals("org.rockbox.UpdateState"))
            {
                CharSequence playstate = args.getCharSequenceExtra("state");
                if (playstate.equals("play"))
                    views.setImageViewResource(R.id.playPause, R.drawable.appwidget_pause);
                else /* pause or stop */
                    views.setImageViewResource(R.id.playPause, R.drawable.appwidget_play);
            }
        }

        appWidgetManager.updateAppWidget(appWidgetId, views);
    }
     
    public static PendingIntent newPendingIntent(Context context, int keycode)
    {
        /* Use keycode as request to code to prevent successive
         * PendingIntents  from overwritting one another.
         * This seems hackish but at least it works.
         * see: http://code.google.com/p/android/issues/detail?id=863
         */
        Intent intent = new Intent(Intent.ACTION_MEDIA_BUTTON, Uri.EMPTY,
                                   context, RockboxService.class);
        intent.putExtra(Intent.EXTRA_KEY_EVENT,
                        new KeyEvent(KeyEvent.ACTION_UP, keycode));
        return PendingIntent.getService(context, keycode, intent, 0);
    }
}


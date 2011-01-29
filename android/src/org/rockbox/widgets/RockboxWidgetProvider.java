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
import android.content.pm.PackageManager;
import android.net.Uri;
import android.util.Log;
import android.view.View;
import android.view.KeyEvent;
import android.widget.RemoteViews;

import java.util.ArrayList;

public class RockboxWidgetProvider extends AppWidgetProvider
{
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
        if (intent.getAction().equals("org.rockbox.TrackUpdateInfo") ||
            intent.getAction().equals("org.rockbox.TrackFinish") ||
            intent.getAction().equals("org.rockbox.UpdateState"))
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

     public static void updateAppWidget(Context context, AppWidgetManager appWidgetManager, int appWidgetId, Intent args)
     {
        AppWidgetProviderInfo provider = appWidgetManager.getAppWidgetInfo(appWidgetId);
        RemoteViews views = null;
        views = new RemoteViews(context.getPackageName(), provider.initialLayout);

        Intent intent = new Intent(context, RockboxActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, intent, 0);
        views.setOnClickPendingIntent(R.id.infoDisplay, pendingIntent);

        RockboxWidgetConfigure.WidgetPref state = RockboxWidgetConfigure.loadWidgetPref(context, appWidgetId);

        if (state.enablePrev)
        {
            intent = new Intent("org.rockbox.Prev", Uri.EMPTY, context, RockboxService.class);
            pendingIntent = PendingIntent.getService(context, 0, intent, 0);
            views.setOnClickPendingIntent(R.id.prev, pendingIntent);
        }
        else
            views.setViewVisibility(R.id.prev, View.GONE);

        if (state.enablePlayPause)
        {
            intent = new Intent("org.rockbox.PlayPause", Uri.EMPTY, context, RockboxService.class);
            pendingIntent = PendingIntent.getService(context, 0, intent, 0);
            views.setOnClickPendingIntent(R.id.playPause, pendingIntent);
        }
        else
            views.setViewVisibility(R.id.playPause, View.GONE);

        if (state.enableNext)
        {
            intent = new Intent("org.rockbox.Next", Uri.EMPTY, context, RockboxService.class);
            pendingIntent = PendingIntent.getService(context, 0, intent, 0);
            views.setOnClickPendingIntent(R.id.next, pendingIntent);
        }
        else
            views.setViewVisibility(R.id.next, View.GONE);

        if (state.enableStop)
        {
            intent = new Intent("org.rockbox.Stop", Uri.EMPTY, context, RockboxService.class);
            pendingIntent = PendingIntent.getService(context, 0, intent, 0);
            views.setOnClickPendingIntent(R.id.stop, pendingIntent);
        }
        else
            views.setViewVisibility(R.id.stop, View.GONE);

        if (args != null)
        {
            if (args.getAction().equals("org.rockbox.TrackUpdateInfo"))
            {
                CharSequence title = args.getCharSequenceExtra("title");
                CharSequence artist = args.getCharSequenceExtra("artist");
                CharSequence album = args.getCharSequenceExtra("album");
                views.setTextViewText(R.id.infoDisplay, title+"\n"+artist+"\n"+album);
            }
            else if (args.getAction().equals("org.rockbox.TrackFinish"))
            {
                // FIXME: looks like this event is always fired earlier than
                // the actual track change (a few seconds)
                views.setTextViewText(R.id.infoDisplay, context.getString(R.string.appwidget_infoDisplay));
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
}


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

import android.app.Activity;
import android.appwidget.AppWidgetManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;


public class RockboxWidgetConfigure extends Activity
{
    int mAppWidgetId = AppWidgetManager.INVALID_APPWIDGET_ID;

    public RockboxWidgetConfigure()
    {
        super();
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        setResult(RESULT_CANCELED);
        setContentView(R.layout.appwidget_configure);

        ((CheckBox)findViewById(R.id.enable_prev)).setChecked(false);
        ((CheckBox)findViewById(R.id.enable_stop)).setChecked(true);
        ((CheckBox)findViewById(R.id.enable_playpause)).setChecked(true);
        ((CheckBox)findViewById(R.id.enable_next)).setChecked(false);

        findViewById(R.id.confirm).setOnClickListener(mCreateWidget);

        Intent intent = getIntent();
        Bundle extras = intent.getExtras();
        if (extras != null)
            mAppWidgetId = extras.getInt(AppWidgetManager.EXTRA_APPWIDGET_ID, AppWidgetManager.INVALID_APPWIDGET_ID);

        if (mAppWidgetId == AppWidgetManager.INVALID_APPWIDGET_ID)
            finish();
    }

    View.OnClickListener mCreateWidget = new View.OnClickListener()
    {
        public void onClick(View v)
        {
            final Context context = RockboxWidgetConfigure.this;

            WidgetPref state = new WidgetPref();
            state.enablePrev = ((CheckBox)findViewById(R.id.enable_prev)).isChecked();
            state.enableStop = ((CheckBox)findViewById(R.id.enable_stop)).isChecked();
            state.enablePlayPause = ((CheckBox)findViewById(R.id.enable_playpause)).isChecked();
            state.enableNext = ((CheckBox)findViewById(R.id.enable_next)).isChecked();
            saveWidgetPref(context, mAppWidgetId, state);

            AppWidgetManager appWidgetManager = AppWidgetManager.getInstance(context);
            RockboxWidgetProvider.updateAppWidget(context, appWidgetManager, mAppWidgetId, null);

            Intent result = new Intent();
            result.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, mAppWidgetId);
            setResult(RESULT_OK, result);
            finish();
        }
    };

    static public class WidgetPref
    {
        public boolean enablePrev = true;
        public boolean enableStop = true;
        public boolean enablePlayPause = true;
        public boolean enableNext = true;
    }

    static void saveWidgetPref(Context context, int appWidgetId, WidgetPref state)
    {
        SharedPreferences.Editor prefs = context.getSharedPreferences("org.rockbox.RockboxWidgetConfigure", 0).edit();
        prefs.putBoolean("prev"+appWidgetId, state.enablePrev);
        prefs.putBoolean("stop"+appWidgetId, state.enableStop);
        prefs.putBoolean("playpause"+appWidgetId, state.enablePlayPause);
        prefs.putBoolean("next"+appWidgetId, state.enableNext);
        prefs.commit();
    }

    static WidgetPref loadWidgetPref(Context context, int appWidgetId)
    {
        SharedPreferences prefs = context.getSharedPreferences("org.rockbox.RockboxWidgetConfigure", 0);
        WidgetPref state = new WidgetPref();
        state.enablePrev = prefs.getBoolean("prev"+appWidgetId, true);
        state.enableStop = prefs.getBoolean("stop"+appWidgetId, true);
        state.enablePlayPause = prefs.getBoolean("playpause"+appWidgetId, true);
        state.enableNext = prefs.getBoolean("next"+appWidgetId, true);
        return state;
    }
}

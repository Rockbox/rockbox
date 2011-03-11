package org.rockbox.Helper;

import java.lang.reflect.Method;

import org.rockbox.R;
import org.rockbox.RockboxActivity;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.util.Log;
import android.widget.RemoteViews;

public class RunForegroundManager
{
    private Notification mNotification;
    private NotificationManager mNM;
    private Service mCurrentService;
    private Intent mWidgetUpdate;

    public RunForegroundManager(Service service)
    {
        mCurrentService = service;
        mNM = (NotificationManager)
                        service.getSystemService(Service.NOTIFICATION_SERVICE);
        RemoteViews views = new RemoteViews(service.getPackageName(), R.layout.statusbar);
        /* create Intent for clicking on the expanded notifcation area */
        Intent intent = new Intent(service, RockboxActivity.class);
        intent = intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        mNotification = new Notification();
        mNotification.tickerText = service.getString(R.string.notification);
        mNotification.icon = R.drawable.notification;
        mNotification.contentView = views;
        mNotification.flags |= Notification.FLAG_ONGOING_EVENT;
        mNotification.contentIntent = PendingIntent.getActivity(service, 0, intent, 0);

        initForegroundCompat();
    }

    private void LOG(CharSequence text)
    {
        Log.d("Rockbox", (String)text);
    }

    private void LOG(CharSequence text, Throwable tr)
    {
        Log.d("Rockbox", (String)text, tr);
    }

    public void startForeground() 
    {
        startForegroundCompat(R.string.notification, mNotification);
    }

    public void stopForeground() 
    {
        stopForegroundCompat(R.string.notification);
        mWidgetUpdate = null;
    }

    public void updateNotification(String title, String artist, String album, String albumart)
    {
        RemoteViews views = mNotification.contentView;
        views.setTextViewText(R.id.title, title);
        views.setTextViewText(R.id.content, artist+"\n"+album);
        if (artist.equals(""))
            mNotification.tickerText = title;
        else
            mNotification.tickerText = title+" - "+artist;
        mNM.notify(R.string.notification, mNotification);

        mWidgetUpdate = new Intent("org.rockbox.TrackUpdateInfo");
        mWidgetUpdate.putExtra("title", title);
        mWidgetUpdate.putExtra("artist", artist);
        mWidgetUpdate.putExtra("album", album);
        mWidgetUpdate.putExtra("albumart", albumart);
        mCurrentService.sendBroadcast(mWidgetUpdate);
    }

    public void resendUpdateNotification()
    {
        if (mWidgetUpdate != null)
            mCurrentService.sendBroadcast(mWidgetUpdate);
    }

    public void finishNotification()
    {
        Log.d("Rockbox", "TrackFinish");
        Intent widgetUpdate = new Intent("org.rockbox.TrackFinish");
        mCurrentService.sendBroadcast(widgetUpdate);
    }

    /* Loosely based on http://developer.android.com/reference/android/app/Service.html#startForeground(int, android.app.Notification) */
    private static final Class<?>[] mSetForegroundSignature = new Class[] {boolean.class};
    private static final Class<?>[] mStartForegroundSignature = new Class[] {int.class, Notification.class};
    private static final Class<?>[] mStopForegroundSignature = new Class[] {boolean.class};

    private Method mSetForeground;
    private Method mStartForeground;
    private Method mStopForeground;

    private void initForegroundCompat() {
        Class<?> serviceClass = mCurrentService.getClass();
        try {
            mStartForeground = serviceClass.getMethod("startForeground", mStartForegroundSignature);
            mStopForeground = serviceClass.getMethod("stopForeground", mStopForegroundSignature);
        } catch (NoSuchMethodException e) {
            // Running on an older platform.
            mStartForeground = mStopForeground = null;
            try {
                mSetForeground = serviceClass.getMethod("setForeground", mSetForegroundSignature);
            } catch (NoSuchMethodException e2) {
                throw new IllegalStateException("OS doesn't have Service.startForeground nor Service.setForeground!", e2);
            }
        }
    }

    private void invokeMethod(Method method, Object[] args) {
        try {
            method.invoke(mCurrentService, args);
        } catch (Exception e) {
            // Should not happen.
            Log.w("Rockbox", "Unable to invoke method", e);
        }
    }

    /**
     * This is a wrapper around the new startForeground method, using the older
     * APIs if it is not available.
     */
    private void startForegroundCompat(int id, Notification notification) {
        // If we have the new startForeground API, then use it.
        if (mStartForeground != null) {
            Object[] startForeGroundArgs = new Object[] {Integer.valueOf(id), notification};
            invokeMethod(mStartForeground, startForeGroundArgs);
        } else {
            // Fall back on the old API.
            Object[] setForegroundArgs = new Object[] {Boolean.TRUE};
            invokeMethod(mSetForeground, setForegroundArgs);
            mNM.notify(id, notification);
        }
    }

    /**
     * This is a wrapper around the new stopForeground method, using the older
     * APIs if it is not available.
     */
    private void stopForegroundCompat(int id) {
        // If we have the new stopForeground API, then use it.
        if (mStopForeground != null) {
            Object[] stopForegroundArgs = new Object[] {Boolean.TRUE};
            invokeMethod(mStopForeground, stopForegroundArgs);
        } else {
            // Fall back on the old API.  Note to cancel BEFORE changing the
            // foreground state, since we could be killed at that point.
            mNM.cancel(id);

            Object[] setForegroundArgs = new Object[] {Boolean.FALSE};
            invokeMethod(mSetForeground, setForegroundArgs);
        }
    }
}

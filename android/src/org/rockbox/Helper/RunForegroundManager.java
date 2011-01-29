package org.rockbox.Helper;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import org.rockbox.R;
import org.rockbox.RockboxActivity;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;
import android.widget.RemoteViews;

public class RunForegroundManager
{
    /* all below is heavily based on the examples found on
     * http://developer.android.com/reference/android/app/Service.html#setForeground(boolean)
     */
    private Notification mNotification;
    private NotificationManager mNM;
    private IRunForeground api;
    private Service mCurrentService;

    public RunForegroundManager(Service service) throws Exception
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

        try {
            api = new newForegroundApi(R.string.notification, mNotification);
        } catch (NoSuchMethodException e) {
            /* Fall back on the old API */
            api = new oldForegroundApi();
        }
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
        /* 
         * Send the notification.
         * We use a layout id because it is a unique number.  
         * We use it later to cancel.
         */
        mNM.notify(R.string.notification, mNotification);
        /*
         * this call makes the service run as foreground, which
         * provides enough cpu time to do music decoding in the 
         * background
         */
        api.startForeground();
    }

    public void stopForeground() 
    {
        /* Note to cancel BEFORE changing the
         * foreground state, since we could be killed at that point.
         */
        mNM.cancel(R.string.notification);
        api.stopForeground();
    }

    public void updateNotification(String title, String artist, String album)
    {
        RemoteViews views = mNotification.contentView;
        views.setTextViewText(R.id.title, title);
        views.setTextViewText(R.id.content, artist+"\n"+album);
        if (artist.equals(""))
            mNotification.tickerText = title;
        else
            mNotification.tickerText = title+" - "+artist;
        mNM.notify(R.string.notification, mNotification);

        Intent widgetUpdate = new Intent("org.rockbox.TrackUpdateInfo");
        widgetUpdate.putExtra("title", title);
        widgetUpdate.putExtra("artist", artist);
        widgetUpdate.putExtra("album", album);
        mCurrentService.sendBroadcast(widgetUpdate);
    }

    public void finishNotification()
    {
        Log.d("Rockbox", "TrackFinish");
        Intent widgetUpdate = new Intent("org.rockbox.TrackFinish");
        mCurrentService.sendBroadcast(widgetUpdate);
    }

    private interface IRunForeground 
    {
        void startForeground();
        void stopForeground();
    }

    private class newForegroundApi implements IRunForeground 
    {
        Class<?>[] mStartForegroundSignature = 
            new Class[] { int.class, Notification.class };
        Class<?>[] mStopForegroundSignature = 
            new Class[] { boolean.class };
        private Method mStartForeground;
        private Method mStopForeground;
        private Object[] mStartForegroundArgs = new Object[2];
        private Object[] mStopForegroundArgs = new Object[1];
        
        newForegroundApi(int id, Notification notification) 
            throws SecurityException, NoSuchMethodException
        {
            /* 
             * Get the new API through reflection
             */
            mStartForeground = Service.class.getMethod("startForeground",
                    mStartForegroundSignature);
            mStopForeground = Service.class.getMethod("stopForeground",
                    mStopForegroundSignature);
            mStartForegroundArgs[0] = id;
            mStartForegroundArgs[1] = notification;
            mStopForegroundArgs[0] = Boolean.TRUE;
        }

        public void startForeground()
        {
            try {
                mStartForeground.invoke(mCurrentService, mStartForegroundArgs);
            } catch (InvocationTargetException e) {
                /* Should not happen. */
                LOG("Unable to invoke startForeground", e);
            } catch (IllegalAccessException e) {
                /* Should not happen. */
                LOG("Unable to invoke startForeground", e);
            }
        }

        public void stopForeground()
        {
            try {
                mStopForeground.invoke(mCurrentService, mStopForegroundArgs);
            } catch (InvocationTargetException e) {
                /* Should not happen. */
                LOG("Unable to invoke stopForeground", e);
            } catch (IllegalAccessException e) {
                /* Should not happen. */
                LOG("Unable to invoke stopForeground", e);
            }
        }
    }
    
    private class oldForegroundApi implements IRunForeground
    {
        public void startForeground()
        {
            mCurrentService.setForeground(false);
        }
        public void stopForeground()
        {
            mCurrentService.setForeground(false);
        }        
    }
}

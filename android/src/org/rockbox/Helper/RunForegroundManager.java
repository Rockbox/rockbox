package org.rockbox.Helper;

import java.lang.reflect.Method;
import org.rockbox.R;
import org.rockbox.RockboxActivity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
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

        try {
            api = new NewForegroundApi(R.string.notification, mNotification);
        } catch (Throwable t) {
            /* Throwable includes Exception and the expected
             * NoClassDefFoundError for Android 1.x */
            try {
                api = new OldForegroundApi();
                Logger.i("RunForegroundManager: Falling back to compatibility API");
            } catch (Exception e) {
                Logger.e("Cannot run in foreground: No available API");
            }
        }
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
        Logger.d("TrackFinish");
        Intent widgetUpdate = new Intent("org.rockbox.TrackFinish");
        mCurrentService.sendBroadcast(widgetUpdate);
    }

    private interface IRunForeground 
    {
        void startForeground();
        void stopForeground();
    }

    private class NewForegroundApi implements IRunForeground 
    {
        int id;
        Notification mNotification;
        NewForegroundApi(int _id, Notification _notification)
        {
            id = _id;
            mNotification = _notification;
        }

        public void startForeground()
        {
            mCurrentService.startForeground(id, mNotification);
        }

        public void stopForeground()
        {
            mCurrentService.stopForeground(true);
        }
    }
    
    private class OldForegroundApi implements IRunForeground
    {
        /* 
         * Get the new API through reflection because it's unavailable 
         * in honeycomb
         */
        private Method mSetForeground;

        public OldForegroundApi() throws SecurityException, NoSuchMethodException
        {
            mSetForeground = getClass().getMethod("setForeground",
                    new Class[] { boolean.class });
        }

        public void startForeground()
        {
            try {
                mSetForeground.invoke(mCurrentService, Boolean.TRUE);
            } catch (Exception e) {
                Logger.e("startForeground compat error: " + e.getMessage());
                e.printStackTrace();
            }
        }
        public void stopForeground()
        {
            try {
                mSetForeground.invoke(mCurrentService, Boolean.FALSE);
            } catch (Exception e) {
                Logger.e("stopForeground compat error: " + e.getMessage());
            }
        }        
    }
}

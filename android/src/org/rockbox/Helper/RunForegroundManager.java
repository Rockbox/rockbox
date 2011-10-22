package org.rockbox.Helper;

import java.lang.reflect.Method;
import org.rockbox.R;
import org.rockbox.RockboxActivity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Drawable;
import android.os.Handler;
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
    private Handler mServiceHandler;
    private Intent mWidgetUpdate;
    private int iconheight;

    public RunForegroundManager(Service service)
    {
        mCurrentService = service;
        mNM = (NotificationManager)
                        service.getSystemService(Service.NOTIFICATION_SERVICE);
        RemoteViews views = new RemoteViews(service.getPackageName(), R.layout.statusbar);
        /* create Intent for clicking on the expanded notifcation area */
        Intent intent = new Intent(service, RockboxActivity.class);
        intent = intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        /* retrieve height of launcher icon. Used to scale down album art. */
        Resources resources = service.getResources();
        Drawable draw = resources.getDrawable(R.drawable.launcher);
        iconheight = draw.getIntrinsicHeight();

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
        mServiceHandler = new Handler(service.getMainLooper());
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

    public void updateNotification(final String title, final String artist, final String album, final String albumart)
    {
        /* do this on the main thread for 2 reasons
         * 1) Don't delay track switching with possibly costly albumart
         *  loading (i.e. off-load from the Rockbox thread)
         * 2) Work around a bug in Android where decodeFile() fails outside
         *  of the main thread (http://stackoverflow.com/q/7228633)
         */
        mServiceHandler.post(new Runnable()
        {
            @Override
            public void run()
            {
                final RemoteViews views = mNotification.contentView;
                views.setTextViewText(R.id.title, title);
                views.setTextViewText(R.id.content, artist+"\n"+album);
                if (artist.equals(""))
                    mNotification.tickerText = title;
                else
                    mNotification.tickerText = title+" - "+artist;

                if (albumart != null) {
                    /* The notification area doesn't have permissions to access the SD card.
                     * Push the data as Bitmap instead of Uri. Scale down to size of
                     * launcher icon -- broadcasting the unscaled image may yield in
                     * too much data, causing UI hangs of Rockbox. */
                    Bitmap b = BitmapFactory.decodeFile(albumart);
                    if(b != null) {
                        /* scale width to keep aspect ratio -- height is the constraint */
                        int scaledwidth = Math.round(iconheight*((float)b.getWidth()/b.getHeight()));
                        views.setImageViewBitmap(R.id.artwork,
                            Bitmap.createScaledBitmap(b, scaledwidth, iconheight, false));
                    }
                    else {
                        views.setImageViewResource(R.id.artwork, R.drawable.launcher);
                    }
                }
                else {
                    views.setImageViewResource(R.id.artwork, R.drawable.launcher);
                }
                mWidgetUpdate = new Intent("org.rockbox.TrackUpdateInfo");
                mWidgetUpdate.putExtra("title", title);
                mWidgetUpdate.putExtra("artist", artist);
                mWidgetUpdate.putExtra("album", album);
                mWidgetUpdate.putExtra("albumart", albumart);
                mCurrentService.sendBroadcast(mWidgetUpdate);
                
                /* notify in this runnable to make sure the notification
                 * has the correct albumart */
                mNM.notify(R.string.notification, mNotification); 
            }
        });       
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

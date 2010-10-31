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
import android.util.Log;

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
        mNM = (NotificationManager)
                        service.getSystemService(Service.NOTIFICATION_SERVICE);
        /* For now we'll use the same text for the ticker and the 
         * expanded notification */
        CharSequence text = service.getText(R.string.notification);
        /* Set the icon, scrolling text and timestamp */
        mNotification = new Notification(R.drawable.icon, text,
                System.currentTimeMillis());

        /* The PendingIntent to launch our activity if the user selects
         * this notification */
        Intent intent = new Intent(service, RockboxActivity.class);
        PendingIntent contentIntent = 
                PendingIntent.getActivity(service, 0, intent, 0);

        /*  Set the info for the views that show in the notification panel. */
        mNotification.setLatestEventInfo(service, 
                service.getText(R.string.notification), text, contentIntent);
        
        try {
            api = new newForegroundApi(R.string.notification, mNotification);
        } catch (NoSuchMethodException e) {
            /* Fall back on the old API */
            api = new oldForegroundApi();
        }
        mCurrentService = service; 
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

package org.rockbox;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

public class RockboxService extends Service 
{
	public static RockboxFramebuffer fb = null;
	@Override
	public void onCreate()
	{
	    mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
		startservice();
	}

	private void do_start(Intent intent)
	{
		LOG("Start Service");
	}

	private void LOG(CharSequence text)
	{
		Log.d("Rockbox", (String) text);
	}
	
	@Override
	public void onStart(Intent intent, int startId) {
		do_start(intent);
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId)
	{
		do_start(intent);
	    /* Display a notification about us starting.  We put an icon in the status bar. */
    	showNotification();
		return START_STICKY;
	}

	private void startservice() 
	{
        fb = new RockboxFramebuffer(this);
		final int BUFFER = 2048;
		/* the following block unzips libmisc.so, which contains the files 
		 * we ship, such as themes. It's needed to put it into a .so file
		 * because there's no other way to ship files and have access
		 * to them from native code
		 */
        try 
        {
           BufferedOutputStream dest = null;
           BufferedInputStream is = null;
           ZipEntry entry;
           File file = new File("/data/data/org.rockbox/lib/libmisc.so");
           /* use arbitary file to determine whether extracting is needed */
           File file2 = new File("/data/data/org.rockbox/app_rockbox/rockbox/codecs/mpa.codec");
           if (!file2.exists() || (file.lastModified() > file2.lastModified()))
           {
	           ZipFile zipfile = new ZipFile(file);
	           Enumeration<? extends ZipEntry> e = zipfile.entries();
	           File folder;
	           while(e.hasMoreElements()) {
	              entry = (ZipEntry) e.nextElement();
	              LOG("Extracting: " +entry);
	              if (entry.isDirectory())
	              {
	            	  folder = new File(entry.getName());
	            	  LOG("mkdir "+ entry);
	            	  try {
	            		  folder.mkdirs();
	            	  } catch (SecurityException ex){
	            		  LOG(ex.getMessage());
	            	  }
	            	  continue;
	              }
	              is = new BufferedInputStream(zipfile.getInputStream(entry));
	              int count;
	              byte data[] = new byte[BUFFER];
	              folder = new File(new File(entry.getName()).getParent());
	              LOG("" + folder.getAbsolutePath());
	              if (!folder.exists())
	            	  folder.mkdirs();
	              FileOutputStream fos = new FileOutputStream(entry.getName());
	              dest = new BufferedOutputStream(fos, BUFFER);
	              while ((count = is.read(data, 0, BUFFER)) != -1) {
	                 dest.write(data, 0, count);
	              }
	              dest.flush();
	              dest.close();
	              is.close();
	           }
           }
        } catch(Exception e) {
           e.printStackTrace();
        }

        System.loadLibrary("rockbox");

    	Thread rb = new Thread(new Runnable()
    	{
    		public void run()
    		{
    			main();
    		}
    	},"Rockbox thread");
    	rb.setDaemon(false);
    	rb.start();
	}

    private native void main();
	@Override
	public IBinder onBind(Intent intent) {
		// TODO Auto-generated method stub
		return null;
	}
    private NotificationManager mNM;

    /**
     * Class for clients to access.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with
     * IPC.
     */
    public class LocalBinder extends Binder {
        RockboxService getService() {
            return RockboxService.this;
        }
    }

    /* heavily based on the example found on
     * http://developer.android.com/reference/android/app/Service.html
     */
	public void showNotification() {
		// In this sample, we'll use the same text for the ticker and the expanded notification
        CharSequence text = getText(R.string.notification);

        // Set the icon, scrolling text and timestamp
        Notification notification = new Notification(R.drawable.rb, text,
                System.currentTimeMillis());

        // The PendingIntent to launch our activity if the user selects this notification
        Intent intent = new Intent(this, RockboxActivity.class);
        PendingIntent contentIntent = PendingIntent.getActivity(this, 0, intent, 0);
        

        // Set the info for the views that show in the notification panel.
        notification.setLatestEventInfo(this, getText(R.string.notification), text, contentIntent);

        // Send the notification.
        // We use a layout id because it is a unique number.  We use it later to cancel.
        mNM.notify(R.string.notification, notification);
        
        /*
         * this call makes the service run as foreground, which
         * provides enough cpu time to do music decoding in the 
         * background
         */
        startForeground(R.string.notification, notification);
	}
}

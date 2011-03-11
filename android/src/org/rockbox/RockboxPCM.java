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

import java.util.Arrays;

import android.content.Intent;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.util.Log;

public class RockboxPCM extends AudioTrack 
{
    private byte[] raw_data;
    private PCMListener l;
    private HandlerThread ht;
    private Handler h = null;
    private static final int samplerate = 44100;
    /* should be CHANNEL_OUT_STEREO in 2.0 and above */
    private static final int channels = 
            AudioFormat.CHANNEL_CONFIGURATION_STEREO;
    private static final int encoding = 
            AudioFormat.ENCODING_PCM_16BIT;
    /* 24k is plenty, but some devices may have a higher minimum */
    private static final int buf_len  = 
            Math.max(24<<10, getMinBufferSize(samplerate, channels, encoding));

    private void LOG(CharSequence text)
    {
        Log.d("Rockbox", (String) text);
    }

    public RockboxPCM()
    {
        super(AudioManager.STREAM_MUSIC, samplerate,  channels, encoding,
                buf_len, AudioTrack.MODE_STREAM);
        ht = new HandlerThread("audio thread", 
                Process.THREAD_PRIORITY_URGENT_AUDIO);
        ht.start();
        raw_data = new byte[buf_len]; /* in shorts */
        Arrays.fill(raw_data, (byte) 0);
        l = new PCMListener(buf_len);
    }    

    private int bytes2frames(int bytes) 
    {
        /* 1 sample is 2 bytes, 2 samples are 1 frame */
        return (bytes/4);
    }
    
    private int frames2bytes(int frames) 
    {
        /* 1 frame is 2 samples, 1 sample is 2 bytes */
        return (frames*4);
    }

    private void play_pause(boolean pause) 
    {
        RockboxService service = RockboxService.get_instance();
        if (pause)
        {
            Intent widgetUpdate = new Intent("org.rockbox.UpdateState");
            widgetUpdate.putExtra("state", "pause");
            service.sendBroadcast(widgetUpdate);
            service.stopForeground();
            pause();
        }
        else
        {
            Intent widgetUpdate = new Intent("org.rockbox.UpdateState");
            widgetUpdate.putExtra("state", "play");
            service.sendBroadcast(widgetUpdate);
            service.startForeground();
            if (getPlayState() == AudioTrack.PLAYSTATE_STOPPED)
            {
                if (getState() == AudioTrack.STATE_INITIALIZED)
                {
                    if (h == null)
                        h = new Handler(ht.getLooper());
                    if (setNotificationMarkerPosition(bytes2frames(buf_len)/4)
                            != AudioTrack.SUCCESS)
                        LOG("setNotificationMarkerPosition Error");
                    else
                        setPlaybackPositionUpdateListener(l, h);
                }
                /* need to fill with silence before starting playback */
                write(raw_data, frames2bytes(getPlaybackHeadPosition()), 
                        raw_data.length);
            }
            play();
        }
    }
    
    @Override
    public void stop() throws IllegalStateException 
    {
        try {
            super.stop();
        } catch (IllegalStateException e) {
            throw new IllegalStateException(e);
        }
        Intent widgetUpdate = new Intent("org.rockbox.UpdateState");
        widgetUpdate.putExtra("state", "stop");
        RockboxService.get_instance().sendBroadcast(widgetUpdate);
        RockboxService.get_instance().stopForeground();
    }

    private void set_volume(int volume)
    {
        /* volume comes from 0..-990 from Rockbox */
        /* TODO:
         * volume is in dB, but this code acts as if it were in %,  convert? */
        float fvolume;
        /* special case min and max volume to not suffer from 
         * floating point accuracy */
        if (volume == 0)
            fvolume = 1.0f;
        else if (volume == -990)
            fvolume = 0.0f;
        else
            fvolume = (volume + 990)/990.0f;
        setStereoVolume(fvolume, fvolume);
    }

    public native void pcmSamplesToByteArray(byte[] dest);
   
    private class PCMListener implements OnPlaybackPositionUpdateListener 
    {
        private int max_len;
        private int refill_mark;
        private byte[] buf;
        public PCMListener(int len) 
        {
            max_len = len;
            /* refill to 100% when reached the 25% */
            buf = new byte[max_len*3/4];
            refill_mark = max_len - buf.length;
        }

        public void onMarkerReached(AudioTrack track) 
        {
            /* push new data to the hardware */
            RockboxPCM pcm = (RockboxPCM)track;
            int result = -1;
            pcm.pcmSamplesToByteArray(buf);
            result = track.write(buf, 0, buf.length);
            if (result >= 0)
            {
                switch(track.getPlayState())
                {
                    case AudioTrack.PLAYSTATE_PLAYING:
                    case AudioTrack.PLAYSTATE_PAUSED:
                        /* refill at 25% no matter of how many 
                         * bytes we've written */
                        if (setNotificationMarkerPosition(
                                bytes2frames(refill_mark)) 
                                    != AudioTrack.SUCCESS)
                        {
                            LOG("Error in onMarkerReached: " +
                            		"Could not set notification marker");
                        }
                        else /* recharge */
                            setPlaybackPositionUpdateListener(this, h);
                        break;
                    case AudioTrack.PLAYSTATE_STOPPED:
                        LOG("State STOPPED");
                        break;
                }
            }
            else
            {
                LOG("Error in onMarkerReached (result="+result+")");
                stop();
            }
        }

        public void onPeriodicNotification(AudioTrack track) 
        {            
        }
    }
}

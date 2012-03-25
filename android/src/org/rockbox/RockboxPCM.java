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
import org.rockbox.Helper.Logger;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;

public class RockboxPCM extends AudioTrack 
{
    private static final int streamtype = AudioManager.STREAM_MUSIC;
    private static final int samplerate = 44100;
    /* should be CHANNEL_OUT_STEREO in 2.0 and above */
    private static final int channels = 
            AudioFormat.CHANNEL_CONFIGURATION_STEREO;
    private static final int encoding = 
            AudioFormat.ENCODING_PCM_16BIT;
    /* 32k is plenty, but some devices may have a higher minimum */
    private static final int buf_len  = 
        Math.max(32<<10, 4*getMinBufferSize(samplerate, channels, encoding));

    private AudioManager audiomanager;
    private RockboxService rbservice;
    private byte[] raw_data;

    private int refillmark;
    private int maxstreamvolume;
    private int setstreamvolume = -1;
    private float minpcmvolume;
    private float curpcmvolume = 0;
    private float pcmrange;

    public RockboxPCM()
    {
        super(streamtype, samplerate,  channels, encoding,
                buf_len, AudioTrack.MODE_STREAM);
        HandlerThread ht = new HandlerThread("audio thread", 
                Process.THREAD_PRIORITY_URGENT_AUDIO);
        ht.start();
        raw_data = new byte[buf_len]; /* in shorts */
        Arrays.fill(raw_data, (byte) 0);

        /* find cleaner way to get context? */
        rbservice = RockboxService.getInstance();
        audiomanager =
            (AudioManager) rbservice.getSystemService(Context.AUDIO_SERVICE);
        maxstreamvolume = audiomanager.getStreamMaxVolume(streamtype);

        minpcmvolume = getMinVolume();
        pcmrange = getMaxVolume() - minpcmvolume;

        setupVolumeHandler();
        postVolume(audiomanager.getStreamVolume(streamtype));
        refillmark = buf_len / 4; /* higher values don't work on many devices */

        /* getLooper() returns null if thread isn't running */
        while(!ht.isAlive()) Thread.yield();
        setPlaybackPositionUpdateListener(
                new PCMListener(buf_len / 2), new Handler(ht.getLooper()));
        refillmark = bytes2frames(refillmark);
    }    

    private native void postVolumeChangedEvent(int volume);

    private void postVolume(int volume)
    {
        int rbvolume = ((maxstreamvolume - volume) * -99) /
            maxstreamvolume;
        Logger.d("java:postVolumeChangedEvent, avol "+volume+" rbvol "+rbvolume);
        postVolumeChangedEvent(rbvolume);
    }
    
    private void setupVolumeHandler()
    {
        BroadcastReceiver broadcastReceiver = new BroadcastReceiver()
        {
            @Override
            public void onReceive(Context context, Intent intent)
            {
                int streamType = intent.getIntExtra(
                    "android.media.EXTRA_VOLUME_STREAM_TYPE", -1);
                int volume = intent.getIntExtra(
                    "android.media.EXTRA_VOLUME_STREAM_VALUE", -1);

                if (streamType == RockboxPCM.streamtype &&
                    volume != -1 &&
                    volume != setstreamvolume &&
                    rbservice.isRockboxRunning())
                {
                    postVolume(volume);
                }
            }
        };

        /* at startup, change the internal rockbox volume to what the global
           android music stream volume is */
        int volume = audiomanager.getStreamVolume(streamtype);
        int rbvolume = ((maxstreamvolume - volume) * -99) / maxstreamvolume;
        postVolumeChangedEvent(rbvolume);

        /* We're relying on internal API's here,
           this can break in the future! */
        rbservice.registerReceiver(
            broadcastReceiver,
            new IntentFilter("android.media.VOLUME_CHANGED_ACTION"));
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
        RockboxService service = RockboxService.getInstance();
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
                setNotificationMarkerPosition(refillmark);
                /* need to fill with silence before starting playback */ 
                write(raw_data, 0, raw_data.length);
            }
            play();
        }
    }
    
    @Override
    public synchronized void stop() throws IllegalStateException 
    {
        /* flush pending data, but turn the volume off so it cannot be heard.
         * This is so that we don't hear old data if music is resumed very
         * quickly after (e.g. when seeking).
         */
        float old_vol = curpcmvolume; 
        try {
            setStereoVolume(0, 0);
            flush();
            super.stop();
        } catch (IllegalStateException e) {
            throw new IllegalStateException(e);
        } finally {
            setStereoVolume(old_vol, old_vol);
        }

        Intent widgetUpdate = new Intent("org.rockbox.UpdateState");
        widgetUpdate.putExtra("state", "stop");
        RockboxService.getInstance().sendBroadcast(widgetUpdate);
        RockboxService.getInstance().stopForeground();
    }
    
    public int setStereoVolume(float leftVolume, float rightVolume)
    {
        curpcmvolume = leftVolume;
        return super.setStereoVolume(leftVolume, rightVolume);
    }

    private void set_volume(int volume)
    {
        Logger.d("java:set_volume("+volume+")");
        /* Rockbox 'volume' is 0..-990 deci-dB attenuation.
           Android streams have rather low resolution volume control,
           typically 8 or 15 steps.
           Therefore we use the pcm volume to add finer steps between
           every android stream volume step.
           It's not "real" dB, but it gives us 100 volume steps.
        */

        float fraction = 1 - (volume / -990.0f);
        int streamvolume = (int)Math.ceil(maxstreamvolume * fraction);
        if (streamvolume > 0) {
            float streamfraction = (float)streamvolume / maxstreamvolume;
            float pcmvolume =
                (fraction / streamfraction) * pcmrange + minpcmvolume;
            setStereoVolume(pcmvolume, pcmvolume);
        }

        int oldstreamvolume = audiomanager.getStreamVolume(streamtype);
        if (streamvolume != oldstreamvolume) {
            Logger.d("java:setStreamVolume("+streamvolume+")");
            setstreamvolume = streamvolume;
            audiomanager.setStreamVolume(streamtype, streamvolume, 0);
        }
    }

    public native int nativeWrite(byte[] temp, int len);
   
    private class PCMListener implements OnPlaybackPositionUpdateListener 
    {
        byte[] pcm_data;
        public PCMListener(int _refill_bufsize) 
        {
            pcm_data = new byte[_refill_bufsize];
        }

        public void onMarkerReached(AudioTrack track) 
        {
            /* push new data to the hardware */
            RockboxPCM pcm = (RockboxPCM)track;
            int result = -1;
            result = pcm.nativeWrite(pcm_data, pcm_data.length);
            if (result >= 0)
            {
                switch(getPlayState())
                {
                    case PLAYSTATE_PLAYING:
                    case PLAYSTATE_PAUSED:
                        setNotificationMarkerPosition(pcm.refillmark);
                        break;
                    case PLAYSTATE_STOPPED:
                        Logger.d("Stopped");
                        break;
                }
            }
            else /* stop on error */ 
                stop();
        }

        public void onPeriodicNotification(AudioTrack track) 
        {            
        }
    }
}

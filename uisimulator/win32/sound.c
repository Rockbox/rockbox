/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2005 by Magnus Holmgren
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "autoconf.h"

#ifdef ROCKBOX_HAS_SIMSOUND

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "sound.h"
#include "kernel.h"
#include "thread-win32.h"
#include "debug.h"

static bool playing = false;

void pcm_play_stop(void);

static void sound_play_chunk(HWAVEOUT wave_out, LPWAVEHDR header, 
    HANDLE event)
{
    unsigned char* buf;
    long len;

    if (!(header->dwFlags & WHDR_DONE) || !sound_get_pcm)
    {
        return;
    }

    EnterCriticalSection(&CriticalSection);
    sound_get_pcm(&buf, &len);
    LeaveCriticalSection(&CriticalSection);

    if (len == 0)
    {
        DEBUGF("simulator got no pcm\n");
        sound_get_pcm = NULL;
        return;
    }

    header->lpData = buf;
    header->dwBufferLength = len;
    header->dwBytesRecorded = 0;
    header->dwUser = 0;
    header->dwFlags = 0;
    header->dwLoops = 1;
    header->lpNext = NULL;
    header->reserved = 0;

    if (MMSYSERR_NOERROR != waveOutPrepareHeader(wave_out, header, 
        sizeof(*header)))
    {
        return;
    }

    ResetEvent(event);

    waveOutWrite(wave_out, header, sizeof(*header));
}

static int sound_init(LPHWAVEOUT wave_out, HANDLE event)
{
    static const WAVEFORMATEX format =
    {
        WAVE_FORMAT_PCM,    /* Format */
        2,                  /* Number of channels */
        44100,              /* Samples per second */
        44100 * 4,          /* Bytes per second */
        4,                  /* Block align */
        16,                 /* Bits per sample */
        0                   /* Extra size */
    };

    if (MMSYSERR_NOERROR != waveOutOpen(wave_out, WAVE_MAPPER, &format, 
        (DWORD_PTR) event, 0, CALLBACK_EVENT))
    {
        return -1;
    }

    /* Full volume on left and right */
    waveOutSetVolume(*wave_out, 0xffffffff);

    return 0;
}

void sound_playback_thread(void)
{
    /* To get smooth playback, two buffers are needed, which are queued for
     * playback. (There can still be glitches though.)
     */
    HWAVEOUT wave_out;
    WAVEHDR header1;
    WAVEHDR header2;
    HANDLE event;
    int result = -1;

    if ((event = CreateEvent(NULL, FALSE, FALSE, NULL)))
    {
        result = sound_init(&wave_out, event);
    }
    
    while(-1 == result || !event)
    {
        Sleep(100000); /* Wait forever, can't play sound! */
    }
    
    while (true)
    {
        while (!sound_get_pcm)
        {
            /* TODO: fix a fine thread-synch mechanism here */
            Sleep(100);
        }

        DEBUGF("starting simulator playback\n");
        header1.dwFlags = WHDR_DONE;
        header2.dwFlags = WHDR_DONE;
        sound_play_chunk(wave_out, &header1, event);
        sound_play_chunk(wave_out, &header2, event);

        while (sound_get_pcm 
            && (WAIT_FAILED != WaitForSingleObject(event, 1000)))
        {
            sound_play_chunk(wave_out, &header1, event);
            sound_play_chunk(wave_out, &header2, event);
        }
        
        pcm_play_stop();
        
        DEBUGF("stopping simulator playback\n");

        waveOutReset(wave_out);
    }
}


/* Stubs for PCM audio playback. */
bool pcm_is_playing(void)
{
    return playing;
}

void pcm_mute(bool state)
{
    (void)state;
}

void pcm_play_pause(bool state)
{
    (void)state;
}

bool pcm_is_paused(void)
{
    return false;
}

void pcm_play_stop(void)
{
    playing = false;
}

void pcm_init(void)
{
}

void (*sound_get_pcm)(unsigned char** start, long* size);
void pcm_play_data(void (*get_more)(unsigned char** start, long* size))
{
    sound_get_pcm = get_more;
    playing = true;
}

long pcm_get_bytes_waiting(void)
{
    return 0;
}


#endif /* ROCKBOX_HAS_SIMSOUND */

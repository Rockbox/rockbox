/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2010 Thomas Martitz
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

#include <jni.h>
#include <stdbool.h>
#define _SYSTEM_WITH_JNI /* for getJavaEnvironment */
#include <system.h>
#include <pthread.h>
#include "debug.h"
#include "pcm.h"
#include "pcm-internal.h"

extern JNIEnv *env_ptr;

/* infos about our pcm chunks */
static const void *pcm_data_start;
static size_t  pcm_data_size;
static int     audio_locked = 0;
static pthread_mutex_t audio_lock_mutex = PTHREAD_MUTEX_INITIALIZER;

/* cache frequently called methods */
static jmethodID play_pause_method;
static jmethodID stop_method;
static jmethodID set_volume_method;
static jmethodID write_method;
static jclass    RockboxPCM_class;
static jobject   RockboxPCM_instance;


/*
 * mutex lock/unlock wrappers neatness' sake
 */
static inline void lock_audio(void)
{
    pthread_mutex_lock(&audio_lock_mutex);
}

static inline void unlock_audio(void)
{
    pthread_mutex_unlock(&audio_lock_mutex);
}


/*
 * write pcm samples to the hardware. Calls AudioTrack.write directly (which
 * is usually a blocking call)
 *
 * temp_array is not strictly needed as a parameter as we could
 * create it here, but that would result in frequent garbage collection
 *
 * it is called from the PositionMarker callback of AudioTrack
 **/
JNIEXPORT jint JNICALL
Java_org_rockbox_RockboxPCM_nativeWrite(JNIEnv *env, jobject this,
                                        jbyteArray temp_array, jint max_size)
{
    bool new_buffer = false;

    lock_audio();

    jint left = max_size;

    if (!pcm_data_size) /* get some initial data */
    {
        new_buffer = pcm_play_dma_complete_callback(PCM_DMAST_OK,
                            &pcm_data_start, &pcm_data_size);
    }

    while(left > 0 && pcm_data_size)
    {
        jint ret;
        jsize transfer_size = MIN((size_t)left, pcm_data_size);
        /* decrement both by the amount we're going to write */
        pcm_data_size -= transfer_size; left -= transfer_size;
        (*env)->SetByteArrayRegion(env, temp_array, 0,
                                        transfer_size, (jbyte*)pcm_data_start);

        ret = (*env)->CallIntMethod(env, this, write_method,
                                            temp_array, 0, transfer_size);

        if (new_buffer)
        {
            new_buffer = false;
            pcm_play_dma_status_callback(PCM_DMAST_STARTED);

            /* NOTE: might need to release the mutex and sleep here if the
               buffer is shorter than the required buffer (like pcm-sdl.c) to
               have the mixer clocked at a regular interval */
        }

        if (ret < 0)
        {
            unlock_audio();
            return ret;
        }

        if (pcm_data_size == 0) /* need new data */
        {
            new_buffer = pcm_play_dma_complete_callback(PCM_DMAST_OK,
                                &pcm_data_start, &pcm_data_size);
        }
        else /* increment data pointer and feed more */
            pcm_data_start += transfer_size;
    }

    if (new_buffer)
        pcm_play_dma_status_callback(PCM_DMAST_STARTED);

    unlock_audio();
    return max_size - left;
}

void pcm_play_lock(void)
{
    if (++audio_locked == 1)
        lock_audio();
}

void pcm_play_unlock(void)
{
    if (--audio_locked == 0)
        unlock_audio();
}

void pcm_dma_apply_settings(void)
{
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    pcm_data_start = addr;
    pcm_data_size = size;
    
    pcm_play_dma_pause(false);
}

void pcm_play_dma_stop(void)
{
    /* NOTE: due to how pcm_play_dma_complete_callback() works, this is
     * possibly called from nativeWrite(), i.e. another (host) thread
     * => need to discover the appropriate JNIEnv* */
    JNIEnv* env = getJavaEnvironment();
    (*env)->CallVoidMethod(env,
                           RockboxPCM_instance,
                           stop_method);
}

void pcm_play_dma_pause(bool pause)
{
    (*env_ptr)->CallVoidMethod(env_ptr,
                               RockboxPCM_instance,
                               play_pause_method,
                               (int)pause);
}

size_t pcm_get_bytes_waiting(void)
{
    return pcm_data_size;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    uintptr_t addr = (uintptr_t)pcm_data_start;
    *count = pcm_data_size / 4;
    return (void *)((addr + 3) & ~3);
}

void pcm_play_dma_init(void)
{
    /* in order to have background music playing after leaving the activity,
     * we need to allocate the PCM object from the Rockbox thread (the Activity
     * runs in a separate thread because it would otherwise kill us when
     * stopping it)
     *
     * Luckily we only reference the PCM object from here, so it's safe (and
     * clean) to allocate it here
     **/
    JNIEnv e = *env_ptr;
    /* get the class and its constructor */
    RockboxPCM_class = e->FindClass(env_ptr, "org/rockbox/RockboxPCM");
    jmethodID constructor = e->GetMethodID(env_ptr, RockboxPCM_class, "<init>", "()V");
    /* instance = new RockboxPCM() */
    RockboxPCM_instance = e->NewObject(env_ptr, RockboxPCM_class, constructor);
    /* cache needed methods */
    play_pause_method = e->GetMethodID(env_ptr, RockboxPCM_class, "play_pause", "(Z)V");
    set_volume_method = e->GetMethodID(env_ptr, RockboxPCM_class, "set_volume", "(I)V");
    stop_method       = e->GetMethodID(env_ptr, RockboxPCM_class, "stop", "()V");
    write_method      = e->GetMethodID(env_ptr, RockboxPCM_class, "write", "([BII)I");
}

void pcm_play_dma_postinit(void)
{
}

void pcm_set_mixer_volume(int volume)
{
    (*env_ptr)->CallVoidMethod(env_ptr, RockboxPCM_instance, set_volume_method, volume);
}

/*
 * release audio resources */
void pcm_shutdown(void)
{
    JNIEnv e = *env_ptr;
    jmethodID release = e->GetMethodID(env_ptr, RockboxPCM_class, "release", "()V");
    e->CallVoidMethod(env_ptr, RockboxPCM_instance, release);
    pthread_mutex_destroy(&audio_lock_mutex);
}

JNIEXPORT void JNICALL
Java_org_rockbox_RockboxPCM_postVolumeChangedEvent(JNIEnv *env,
                                                   jobject this,
                                                   jint volume)
{
    (void) env;
    (void) this;
    /* for the main queue, the volume will be available through
     * button_get_data() */
    queue_broadcast(SYS_VOLUME_CHANGED, volume);
}

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
#include <system.h>
#include "debug.h"
#include "pcm.h"

/* infos about our pcm chunks */
static size_t  pcm_data_size;
static char   *pcm_data_start;

/* cache frequently called methods */
static jmethodID play_pause_method;
static jmethodID stop_method;
static jmethodID set_volume_method;
static jclass    RockboxPCM_class;
static jobject   RockboxPCM_instance;


/*
 * transfer our raw data into a java array
 *
 * a bit of a monster functions, but it should cover all cases to overcome
 * the issue that the chunk size of the java layer and our pcm chunks are
 * differently sized
 *
 * afterall, it only copies the raw pcm data from pcm_data_start to
 * the passed byte[]-array
 *
 * it is called from the PositionMarker callback of AudioTrack
 **/
JNIEXPORT void JNICALL
Java_org_rockbox_RockboxPCM_pcmSamplesToByteArray(JNIEnv *env,
                                                  jobject this,
                                                  jbyteArray arr)
{
    (void)this;
    size_t len;
	size_t array_size = (*env)->GetArrayLength(env, arr);
    if (array_size > pcm_data_size)
        len = pcm_data_size;
    else
        len = array_size;

	(*env)->SetByteArrayRegion(env, arr, 0, len, pcm_data_start);

    if (array_size > pcm_data_size)
    {   /* didn't have enough data for the array ? */
        size_t remaining = array_size - pcm_data_size;
        size_t offset = len;
    retry:
        pcm_play_get_more_callback((void**)&pcm_data_start, &pcm_data_size);
        if (pcm_data_size == 0)
        {
            DEBUGF("out of data\n");
            return;
        }
        if (remaining > pcm_data_size)
        {   /* got too little data, get more ... */
            (*env)->SetByteArrayRegion(env, arr, offset, pcm_data_size, pcm_data_start);
            /* advance in the java array by the amount we copied */
            offset += pcm_data_size;
            /* we copied at least a bit */
            remaining -= pcm_data_size;
            pcm_data_size = 0;
            /* let's get another buch of data and try again */
            goto retry;
        }
        else
            (*env)->SetByteArrayRegion(env, arr, offset, remaining, pcm_data_start);
        len = remaining;
    }
    pcm_data_start += len;
    pcm_data_size -= len;
}

void pcm_play_lock(void)
{
}

void pcm_play_unlock(void)
{
}

void pcm_dma_apply_settings(void)
{
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    pcm_data_start = (char*)addr;
    pcm_data_size = size;
    
    pcm_play_dma_pause(false);
}

void pcm_play_dma_stop(void)
{
    JNIEnv *env_ptr = getJavaEnvironment();

    (*env_ptr)->CallVoidMethod(env_ptr,
                               RockboxPCM_instance,
                               stop_method);
}

void pcm_play_dma_pause(bool pause)
{
    JNIEnv *env_ptr = getJavaEnvironment();

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
    JNIEnv *env_ptr = getJavaEnvironment();
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
    /* get initial pcm data, if any */
    pcm_play_get_more_callback((void*)&pcm_data_start, &pcm_data_size);
}

void pcm_postinit(void)
{
}

void pcm_set_mixer_volume(int volume)
{
    JNIEnv *env_ptr = getJavaEnvironment();

    (*env_ptr)->CallVoidMethod(env_ptr, RockboxPCM_instance, set_volume_method, volume);
}

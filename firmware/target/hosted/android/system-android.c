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


#include <setjmp.h>
#include <jni.h>
#include <pthread.h>
#if defined(DX50) || defined(DX90)
#include <stdlib.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <stdio.h>
#endif /* DX50 || DX90 */
#include <unistd.h>
#include "config.h"
#include "system.h"
#include "power.h"
#include "button.h"



#if !defined(DX50) && !defined(DX90)
/* global fields for use with various JNI calls */
static JavaVM *vm_ptr;
JNIEnv *env_ptr;
jobject RockboxService_instance;
jclass  RockboxService_class;
#endif /* !DX50 && !DX90 */

uintptr_t *stackbegin;
uintptr_t *stackend;

extern int main(void);
#if !defined(DX50) && !defined(DX90)
extern void telephony_init_device(void);
#endif
void system_exception_wait(void)
{
#if defined(DX50) || defined(DX90)
    while(1);
#else
    intptr_t dummy = 0;
    while(button_read_device(&dummy) != BUTTON_BACK);
#endif /* DX50 || DX90 */
}

void system_reboot(void)
{
#if defined(DX50) || defined(DX90)
    reboot(RB_AUTOBOOT);
#else
    power_off();
#endif /* DX50 || DX90 */
}

#if !defined(DX50) && !defined(DX90)
/* this is used to return from the entry point of the native library. */
static jmp_buf poweroff_buf;
#endif

void power_off(void)
{
#if defined(DX50) || defined(DX90)
    reboot(RB_POWER_OFF);
#else
    longjmp(poweroff_buf, 1);
#endif /* DX50 || DX90 */
}

void system_init(void)
{
#if defined(DX50) || defined(DX90)
     volatile uintptr_t stack = 0;
     stackbegin = stackend = (uintptr_t*) &stack;

     struct stat m1, m2;
     stat("/mnt/", &m1);
     do
     {
         /* waiting for storage to get mounted */
         stat("/sdcard/", &m2);
         usleep(100000);
     }
     while(m1.st_dev == m2.st_dev);
/* here would be the correct place for 'system("/system/bin/muteopen");' (headphone-out relay) but in pcm-dx50.c, pcm_play_dma_start()
   the output capacitors are charged already a bit and the click of the headphone-connection-relay is softer */

#if defined(DX90)
    /* DAC needs to be unmuted on DX90 */
    FILE * f = fopen("/sys/class/codec/wm8740_mute", "w");
    fputc(0, f);
    fclose(f);
#endif /* DX90 */

#else
    /* no better place yet */
    telephony_init_device();
#endif /* DX50 || DX90 */
}

int hostfs_init(void)
{
    /* stub */
    return 0;
}

int hostfs_flush(void)
{
    sync();
    return 0;
}

#if !defined(DX50) && !defined(DX90)
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void* reserved)
{
    (void)reserved;
    vm_ptr = vm;

    return JNI_VERSION_1_2;
}

JNIEnv* getJavaEnvironment(void)
{
    JNIEnv* env;
    (*vm_ptr)->GetEnv(vm_ptr, (void**) &env, JNI_VERSION_1_2);
    return env;
}

/* this is the entry point of the android app initially called by jni */
JNIEXPORT void JNICALL
Java_org_rockbox_RockboxService_main(JNIEnv *env, jobject this)
{
    /* hack!!! we can't have a valid stack pointer otherwise.
     * but we don't really need it anyway, thread.c only needs it
     * for overflow detection which doesn't apply for the main thread
     * (it's managed by the OS) */

    volatile uintptr_t stack = 0;
    stackbegin = stackend = (uintptr_t*) &stack;
    /* setup a jmp_buf to come back later in case of exit */
    if (setjmp(poweroff_buf) == 0)
    {
        env_ptr = env;

        RockboxService_instance = this;
        RockboxService_class = (*env)->GetObjectClass(env, this);

        main();
    }
    /* simply return here. this will allow the VM to clean up objects and do
     * garbage collection */
}
#endif /* !DX50 && !DX90 */

/* below is the facility for external (from other java threads) to safely call
 * into our snative code. When extracting rockbox.zip the main function is
 * called only after extraction. This delay can be accounted for by calling
 * wait_rockbox_ready(). This does not return until the critical parts of Rockbox
 * can be considered up and running. */
static pthread_cond_t btn_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t btn_mtx = PTHREAD_MUTEX_INITIALIZER;
static bool initialized;

bool is_rockbox_ready(void)
{
    /* don't bother with mutexes for this */
    return initialized;
}

void wait_rockbox_ready(void)
{
    pthread_mutex_lock(&btn_mtx);

    if (!initialized)
        pthread_cond_wait(&btn_cond, &btn_mtx);

    pthread_mutex_unlock(&btn_mtx);
}

void set_rockbox_ready(void)
{
    pthread_mutex_lock(&btn_mtx);

    initialized = true;
    /* now ready. signal all waiters */
    pthread_cond_broadcast(&btn_cond);

    pthread_mutex_unlock(&btn_mtx);
}

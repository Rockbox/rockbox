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

#include <jni.h>
#include <stdio.h>
#include <sys/mman.h>
#include "notification.h"
#include "appevents.h"
#include "metadata.h"
#include "albumart.h"
#include "misc.h"
#include "thread.h"
#include "debug.h"

extern JNIEnv *env_ptr;
extern jclass RockboxService_class;
extern jobject RockboxService_instance;

static jmethodID updateNotification, finishNotification;
static jobject NotificationManager_instance;
static jstring title, artist, album, albumart;

/* completely arbitrary dimensions. neded for find_albumart() */
static const struct dim dim = { .width = 200, .height = 200 };
#define NZV(a) (a && a[0])

/*
 * notify about track change, and show track info */
static void track_changed_callback(void *param)
{
    struct mp3entry* id3 = (struct mp3entry*)param;
    JNIEnv e = *env_ptr;
    if (id3)
    {
        /* passing NULL to DeleteLocalRef() is OK */
        e->DeleteLocalRef(env_ptr, title);
        e->DeleteLocalRef(env_ptr, artist);
        e->DeleteLocalRef(env_ptr, album);
        e->DeleteLocalRef(env_ptr, albumart);

        char buf[200];
        const char * ptitle = id3->title;
        if (!ptitle && *id3->path)
        {   /* pass the filename as title if id3 info isn't available */
            ptitle = strip_extension(buf, sizeof(buf), strrchr(id3->path,'/') + 1);
        }

        title = e->NewStringUTF(env_ptr, ptitle ?: "");
        artist = e->NewStringUTF(env_ptr, id3->artist ?: "");
        album = e->NewStringUTF(env_ptr, id3->album ?: "");

        albumart = NULL;
        if (id3->has_embedded_albumart && id3->albumart.type == AA_TYPE_JPG)
        {   /* extract albumart to a temporary file using mmap() */
            snprintf(buf, sizeof(buf), "/sdcard/rockbox/.temp_albumart_%d.jpg",
                     thread_self());
            int dst_fd = creat(buf, 0666);
            if (dst_fd >= 0)
            {
                int src_fd = open(id3->path, O_RDONLY);
                off_t o_pos = id3->albumart.pos;
                off_t pa_pos = o_pos & ~(sysconf(_SC_PAGE_SIZE) - 1);
                if (src_fd >= 0)
                {   /* align to page boundary */
                    int pos_diff = o_pos - pa_pos;
                    unsigned char* p = mmap(NULL, id3->albumart.size + pos_diff,
                                        PROT_READ, MAP_SHARED, src_fd, pa_pos);
                    if (p != MAP_FAILED)
                    {
                        write(dst_fd, p + pos_diff, id3->albumart.size);
                        munmap(p, id3->albumart.size + pos_diff);
                        albumart = e->NewStringUTF(env_ptr, buf);
                    }
                    close(src_fd);
                }
                close(dst_fd);
            }
        }
        else if (find_albumart(id3, buf, sizeof(buf), &dim))
        {
            albumart = e->NewStringUTF(env_ptr, buf);
        }

        e->CallVoidMethod(env_ptr, NotificationManager_instance,
                      updateNotification, title, artist, album, albumart);
    }
}

/*
 * notify about track finishing */
static void track_finished_callback(void *param)
{
    (void)param;
    JNIEnv e = *env_ptr;
    e->CallVoidMethod(env_ptr, NotificationManager_instance,
                      finishNotification);

    /* delete temporary albumart file */
    char buf[MAX_PATH];
    snprintf(buf, sizeof(buf), "/sdcard/rockbox/.temp_albumart_%d.jpg",
                     thread_self());
    unlink(buf);
}

void notification_init(void)
{
    JNIEnv e = *env_ptr;
    jfieldID nNM = e->GetFieldID(env_ptr, RockboxService_class,
                    "mFgRunner", "Lorg/rockbox/Helper/RunForegroundManager;");
    NotificationManager_instance = e->GetObjectField(env_ptr,
                                                RockboxService_instance, nNM);
    if (NotificationManager_instance == NULL)
    {
        DEBUGF("Failed to get RunForegroundManager instance. Performance will be bad");
        return;
    }

    jclass class = e->GetObjectClass(env_ptr, NotificationManager_instance);
    updateNotification = e->GetMethodID(env_ptr, class, "updateNotification",
                                         "(Ljava/lang/String;"
                                         "Ljava/lang/String;"
                                         "Ljava/lang/String;"
                                         "Ljava/lang/String;)V");
    finishNotification = e->GetMethodID(env_ptr, class, "finishNotification",
                                        "()V");

    add_event(PLAYBACK_EVENT_TRACK_CHANGE, false, track_changed_callback);
    add_event(PLAYBACK_EVENT_TRACK_FINISH, false, track_finished_callback);
}

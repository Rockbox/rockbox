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
#include "notification.h"
#include "appevents.h"
#include "metadata.h"
#include "misc.h"

extern JNIEnv *env_ptr;
extern jclass RockboxService_class;
extern jobject RockboxService_instance;

static jmethodID updateNotification;
static jobject NotificationManager_instance;
static jstring headline, content, ticker;

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
        e->DeleteLocalRef(env_ptr, headline);
        e->DeleteLocalRef(env_ptr, content);
        e->DeleteLocalRef(env_ptr, ticker);

        char buf[200];
        const char * title = id3->title;
        if (!title)
        {   /* pass the filename as title if id3 info isn't available */
            title =
                strip_extension(buf, sizeof(buf), strrchr(id3->path,'/') + 1);
        }

        /* do text for the notification area in the scrolled-down statusbar */
        headline = e->NewStringUTF(env_ptr, title);

        /* add a \n so that android does split into two lines */
        snprintf(buf, sizeof(buf), "%s\n%s", id3->artist ?: "", id3->album ?: "");
        content = e->NewStringUTF(env_ptr, buf);

        /* now do the text for the notification in the statusbar itself */
        if (NZV(id3->artist))
        {   /* title - artist */
            snprintf(buf, sizeof(buf), "%s - %s", title, id3->artist);
            ticker = e->NewStringUTF(env_ptr, buf);
        }
        else
        {   /* title */
            ticker = e->NewStringUTF(env_ptr, title);
        }
        e->CallVoidMethod(env_ptr, NotificationManager_instance,
                      updateNotification, headline, content, ticker);
    }
}

void notification_init(void)
{
    JNIEnv e = *env_ptr;
    jfieldID nNM = e->GetFieldID(env_ptr, RockboxService_class,
                    "fg_runner", "Lorg/rockbox/Helper/RunForegroundManager;");
    NotificationManager_instance = e->GetObjectField(env_ptr,
                                                RockboxService_instance, nNM);

    jclass class = e->GetObjectClass(env_ptr, NotificationManager_instance);
    updateNotification = e->GetMethodID(env_ptr, class, "updateNotification",
                                         "(Ljava/lang/String;"
                                         "Ljava/lang/String;"
                                         "Ljava/lang/String;)V");
    add_event(PLAYBACK_EVENT_TRACK_CHANGE, false, track_changed_callback);
}

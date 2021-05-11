/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#include "jztool_private.h"
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#ifdef WIN32
# include <windows.h>
#endif

/** \brief Allocate a library context
 * \returns New context or NULL if out of memory
 */
jz_context* jz_context_create(void)
{
    jz_context* jz = malloc(sizeof(struct jz_context));
    if(!jz)
        return NULL;

    memset(jz, 0, sizeof(struct jz_context));
    jz->log_level = JZ_LOG_ERROR;
    return jz;
}

/** \brief Destroy the context and free its memory */
void jz_context_destroy(jz_context* jz)
{
    if(jz->usb_ctx) {
        jz_log(jz, JZ_LOG_ERROR, "BUG: USB was not cleaned up properly");
        libusb_exit(jz->usb_ctx);
    }

    free(jz);
}

/** \brief Set a user data pointer. Useful for callbacks. */
void jz_context_set_user_data(jz_context* jz, void* ptr)
{
    jz->user_data = ptr;
}

/** \brief Get the user data pointer */
void* jz_context_get_user_data(jz_context* jz)
{
    return jz->user_data;
}

/** \brief Set the log message callback.
 * \note By default, no message callback is set! No messages will be logged
 *       in this case, so ensure you set a callback if messages are desired.
 */
void jz_context_set_log_cb(jz_context* jz, jz_log_cb cb)
{
    jz->log_cb = cb;
}

/** \brief Set the log level.
 *
 * Messages of less importance than the set log level are not logged.
 * The default log level is `JZ_LOG_WARNING`. The special log level
 * `JZ_LOG_IGNORE` can be used to disable all logging temporarily.
 *
 * The `JZ_LOG_DEBUG` log level is extremely verbose and will log all calls,
 * normally it's only useful for catching bugs.
 */
void jz_context_set_log_level(jz_context* jz, jz_log_level lev)
{
    jz->log_level = lev;
}

/** \brief Log an informational message.
 * \param lev   Log level for this message
 * \param fmt   `printf` style message format string
 */
void jz_log(jz_context* jz, jz_log_level lev, const char* fmt, ...)
{
    if(!jz->log_cb)
        return;
    if(lev == JZ_LOG_IGNORE)
        return;
    if(lev > jz->log_level)
        return;

    va_list ap;

    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if(n < 0)
        return;

    char* buf = malloc(n + 1);
    if(!buf)
        return;

    va_start(ap, fmt);
    n = vsnprintf(buf, n + 1, fmt, ap);
    va_end(ap);

    if(n >= 0)
        jz->log_cb(lev, buf);

    free(buf);
}

/** \brief Log callback which writes messages to `stderr`.
 */
void jz_log_cb_stderr(jz_log_level lev, const char* msg)
{
    static const char* const tags[] =
        {"ERROR", "WARNING", "NOTICE", "DETAIL", "DEBUG"};
    fprintf(stderr, "[%7s] %s\n", tags[lev], msg);
    fflush(stderr);
}

/** \brief Sleep for `ms` milliseconds.
 */
void jz_sleepms(int ms)
{
#ifdef WIN32
    Sleep(ms);
#else
    struct timespec ts;
    long ns = ms % 1000;
    ts.tv_nsec = ns * 1000 * 1000;
    ts.tv_sec = ms / 1000;
    nanosleep(&ts, NULL);
#endif
}

/** \brief Add reference to libusb context, allocating it if necessary */
int jz_context_ref_libusb(jz_context* jz)
{
    if(jz->usb_ctxref == 0) {
        int rc = libusb_init(&jz->usb_ctx);
        if(rc < 0) {
            jz_log(jz, JZ_LOG_ERROR, "libusb_init: %s", libusb_strerror(rc));
            return JZ_ERR_USB;
        }
    }

    jz->usb_ctxref += 1;
    return JZ_SUCCESS;
}

/** \brief Remove reference to libusb context, freeing if it hits zero */
void jz_context_unref_libusb(jz_context* jz)
{
    if(--jz->usb_ctxref == 0) {
        libusb_exit(jz->usb_ctx);
        jz->usb_ctx = NULL;
    }
}

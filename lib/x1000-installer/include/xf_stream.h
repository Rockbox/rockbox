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

#ifndef _XF_STREAM_H_
#define _XF_STREAM_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h> /* ssize_t */
#include "microtar.h"

struct xf_stream;

struct xf_stream_ops {
    off_t(*xget_size)(struct xf_stream* s);
    ssize_t(*xread)(struct xf_stream* s, void* buf, size_t len);
    ssize_t(*xwrite)(struct xf_stream* s, const void* buf, size_t len);
    int(*xclose)(struct xf_stream* s);
};

struct xf_stream {
    intptr_t data;
    const struct xf_stream_ops* ops;
};

inline size_t xf_stream_get_size(struct xf_stream* s)
{ return s->ops->xget_size(s); }

inline ssize_t xf_stream_read(struct xf_stream* s, void* buf, size_t len)
{ return s->ops->xread(s, buf, len); }

inline ssize_t xf_stream_write(struct xf_stream* s, const void* buf, size_t len)
{ return s->ops->xwrite(s, buf, len); }

inline int xf_stream_close(struct xf_stream* s)
{ return s->ops->xclose(s); }

int xf_open_file(const char* file, int flags, struct xf_stream* s);
int xf_open_tar(mtar_t* mtar, const char* file, struct xf_stream* s);
int xf_create_tar(mtar_t* mtar, const char* file, size_t size, struct xf_stream* s);

/* Utility function needed for a few things */
int xf_stream_read_lines(struct xf_stream* s, char* buf, size_t bufsz,
                         int(*callback)(int n, char* buf, void* arg), void* arg);

#endif /* _XF_STREAM_H_ */

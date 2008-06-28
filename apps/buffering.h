/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Nicolas Pennequin
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

#ifndef _BUFFERING_H_
#define _BUFFERING_H_

#include <sys/types.h>
#include <stdbool.h>
#include "events.h"


enum data_type {
    TYPE_CODEC,
    TYPE_PACKET_AUDIO,
    TYPE_ATOMIC_AUDIO,
    TYPE_ID3,
    TYPE_CUESHEET,
    TYPE_BITMAP,
    TYPE_BUFFER,
    TYPE_UNKNOWN,
};

enum callback_event {
    EVENT_BUFFER_LOW = (EVENT_CLASS_BUFFERING|1),
    EVENT_HANDLE_REBUFFER,
    EVENT_HANDLE_CLOSED,
    EVENT_HANDLE_MOVED,
    EVENT_HANDLE_FINISHED,
};

/* Error return values */
#define ERR_HANDLE_NOT_FOUND    -1
#define ERR_BUFFER_FULL         -2
#define ERR_INVALID_VALUE       -3
#define ERR_FILE_ERROR          -4
#define ERR_HANDLE_NOT_DONE     -5


/* Initialise the buffering subsystem */
void buffering_init(void);

/* Reset the buffering system */
bool buffering_reset(char *buf, size_t buflen);


/***************************************************************************
 * MAIN BUFFERING API CALLS
 * ========================
 *
 * bufopen   : Reserve space in the buffer for a given file
 * bufalloc  : Open a new handle from data that needs to be copied from memory
 * bufclose  : Close an open handle
 * bufseek   : Set handle reading index, relatively to the start of the file
 * bufadvance: Move handle reading index, relatively to current position
 * bufread   : Copy data from a handle to a buffer
 * bufgetdata: Obtain a pointer for linear access to a "size" amount of data
 * bufgettail: Out-of-band get the last size bytes of a handle.
 * bufcuttail: Out-of-band remove the trailing 'size' bytes of a handle.
 *
 * NOTE: bufread and bufgetdata will block the caller until the requested
 * amount of data is ready (unless EOF is reached).
 * NOTE: Tail operations are only legal when the end of the file is buffered.
 ****************************************************************************/

#define BUF_MAX_HANDLES         256

int bufopen(const char *file, size_t offset, enum data_type type);
int bufalloc(const void *src, size_t size, enum data_type type);
bool bufclose(int handle_id);
int bufseek(int handle_id, size_t newpos);
int bufadvance(int handle_id, off_t offset);
ssize_t bufread(int handle_id, size_t size, void *dest);
ssize_t bufgetdata(int handle_id, size_t size, void **data);
ssize_t bufgettail(int handle_id, size_t size, void **data);
ssize_t bufcuttail(int handle_id, size_t size);


/***************************************************************************
 * SECONDARY FUNCTIONS
 * ===================
 *
 * buf_get_offset: Get a handle offset from a pointer
 * buf_handle_offset: Get the offset of the first buffered byte from the file
 * buf_request_buffer_handle: Request buffering of a handle
 * buf_set_base_handle: Tell the buffering thread which handle is currently read
 * buf_used: Total amount of buffer space used (including allocated space)
 ****************************************************************************/

ssize_t buf_get_offset(int handle_id, void *ptr);
ssize_t buf_handle_offset(int handle_id);
void buf_request_buffer_handle(int handle_id);
void buf_set_base_handle(int handle_id);
size_t buf_used(void);



/* Settings */
enum {
    BUFFERING_SET_WATERMARK = 1,
    BUFFERING_SET_CHUNKSIZE,
};
void buf_set_watermark(size_t bytes);


/* Debugging */
struct buffering_debug {
    int num_handles;
    size_t buffered_data;
    size_t wasted_space;
    size_t data_rem;
    size_t useful_data;
};
void buffering_get_debugdata(struct buffering_debug *dbgdata);

#endif

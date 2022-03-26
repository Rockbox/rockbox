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
#include "config.h"
#include "appevents.h"


enum data_type {
    TYPE_UNKNOWN = 0, /* invalid type indicator */
    TYPE_ID3,
    TYPE_CODEC,
    TYPE_PACKET_AUDIO,
    TYPE_ATOMIC_AUDIO,
    TYPE_CUESHEET,
    TYPE_BITMAP,
    TYPE_RAW_ATOMIC,
};

/* Error return values */
#define ERR_HANDLE_NOT_FOUND    -1
#define ERR_BUFFER_FULL         -2
#define ERR_INVALID_VALUE       -3
#define ERR_FILE_ERROR          -4
#define ERR_HANDLE_NOT_DONE     -5
#define ERR_UNSUPPORTED_TYPE    -6
#define ERR_BITMAP_TOO_LARGE    -7

/* Initialise the buffering subsystem */
void buffering_init(void) INIT_ATTR;

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
 * bufftell  : Return the handle's file read position
 * bufread   : Copy data from a handle to a buffer
 * bufgetdata: Obtain a pointer for linear access to a "size" amount of data
 *
 * NOTE: bufread and bufgetdata will block the caller until the requested
 * amount of data is ready (unless EOF is reached).
 * NOTE: Tail operations are only legal when the end of the file is buffered.
 ****************************************************************************/

int bufopen(const char *file, off_t offset, enum data_type type,
            void *user_data);
int bufalloc(const void *src, size_t size, enum data_type type);
bool bufclose(int handle_id);
int bufseek(int handle_id, size_t newpos);
int bufadvance(int handle_id, off_t offset);
off_t bufftell(int handle_id);
ssize_t bufread(int handle_id, size_t size, void *dest);
ssize_t bufgetdata(int handle_id, size_t size, void **data);

/***************************************************************************
 * SECONDARY FUNCTIONS
 * ===================
 *
 * buf_handle_data_type: return the handle's data type
 * buf_is_handle: is the handle valid?
 * buf_pin_handle: Disallow/allow handle movement. Handle may still be removed.
 * buf_handle_offset: Get the offset of the first buffered byte from the file
 * buf_set_base_handle: Tell the buffering thread which handle is currently read
 * buf_length: Total size of ringbuffer
 * buf_used: Total amount of buffer space used (including allocated space)
 * buf_back_off_storage: tell buffering thread to take it easy
 ****************************************************************************/

bool buf_is_handle(int handle_id);
int buf_handle_data_type(int handle_id);
off_t buf_filesize(int handle_id);
off_t buf_handle_offset(int handle_id);
off_t buf_handle_remaining(int handle_id);
bool buf_pin_handle(int handle_id, bool pin);
bool buf_signal_handle(int handle_id, bool signal);

size_t buf_length(void);
size_t buf_used(void);

/* Settings */
void buf_set_base_handle(int handle_id);
void buf_set_watermark(size_t bytes);
size_t buf_get_watermark(void);

/* Debugging */
struct buffering_debug {
    int num_handles;
    size_t buffered_data;
    size_t data_rem;
    size_t useful_data;
    size_t watermark;
};
void buffering_get_debugdata(struct buffering_debug *dbgdata);

#endif

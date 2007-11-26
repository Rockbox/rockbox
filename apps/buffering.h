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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _BUFFERING_H_
#define _BUFFERING_H_

#include <sys/types.h>
#include <stdbool.h>


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
    EVENT_DEFAULT,
    EVENT_BUFFER_LOW,
    EVENT_HANDLE_REBUFFER,
    EVENT_HANDLE_CLOSED,
    EVENT_HANDLE_MOVED,
};

/* Error return values */
#define ERR_HANDLE_NOT_FOUND    -1
#define ERR_BUFFER_FULL         -2
#define ERR_INVALID_VALUE       -3
#define ERR_FILE_ERROR          -4


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
 *
 * NOTE: bufread and bufgetdata will block the caller until the requested
 * amount of data is ready (unless EOF is reached).
 ****************************************************************************/

#define BUF_MAX_HANDLES         256

int bufopen(const char *file, size_t offset, enum data_type type);
int bufalloc(const void *src, size_t size, enum data_type type);
bool bufclose(int handle_id);
int bufseek(int handle_id, size_t newpos);
int bufadvance(int handle_id, off_t offset);
ssize_t bufread(int handle_id, size_t size, void *dest);
ssize_t bufgetdata(int handle_id, size_t size, void **data);


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


/***************************************************************************
 * CALLBACK UTILITIES
 * ==================
 *
 * register_buffering_callback, unregister_buffering_callback:
 *
 * Register/Unregister callback functions that will get executed when the buffer
 * goes below the low watermark. They are executed once, then forgotten.
 *
 * NOTE: The callbacks are called from the buffering thread, so don't make them
 * do too much. Ideally they should just post an event to a queue and return.
 ****************************************************************************/

#define MAX_BUF_CALLBACKS 4
typedef void (*buffering_callback)(enum callback_event ev, int value);
bool register_buffering_callback(buffering_callback func);
void unregister_buffering_callback(buffering_callback func);

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

/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
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
#ifndef MISC_H
#define MISC_H

#include <stdbool.h>
#include <inttypes.h>
#include "config.h"

/* Format a large-range value for output, using the appropriate unit so that
 * the displayed value is in the range 1 <= display < 1000 (1024 for "binary"
 * units) if possible, and 3 significant digits are shown. If a buffer is
 * given, the result is snprintf()'d into that buffer, otherwise the result is
 * voiced.*/
char *output_dyn_value(char *buf, int buf_size, int value,
                      const unsigned char **units, bool bin_scale);

/* Create a filename with a number part in a way that the number is 1
 * higher than the highest numbered file matching the same pattern.
 * It is allowed that buffer and path point to the same memory location,
 * saving a strcpy(). Path must always be given without trailing slash.
 *
 * "num" can point to an int specifying the number to use or NULL or a value
 * less than zero to number automatically. The final number used will also
 * be returned in *num. If *num is >= 0 then *num will be incremented by
 * one. */
#if defined(HAVE_RECORDING) && (CONFIG_RTC == 0)
/* this feature is needed by recording without a RTC to prevent disk access
   when changing files */
#define IF_CNFN_NUM_(...) __VA_ARGS__
#define IF_CNFN_NUM
#else
#define IF_CNFN_NUM_(...)
#endif
char *create_numbered_filename(char *buffer, const char *path, 
                               const char *prefix, const char *suffix,
                               int numberlen IF_CNFN_NUM_(, int *num));

/* Format time into buf.
 *
 * buf      - buffer to format to.
 * buf_size - size of buffer.
 * t        - time to format, in milliseconds.
 */
void format_time(char* buf, int buf_size, long t);

#if CONFIG_RTC
/* Create a filename with a date+time part.
   It is allowed that buffer and path point to the same memory location,
   saving a strcpy(). Path must always be given without trailing slash.
   unique_time as true makes the function wait until the current time has
   changed. */
char *create_datetime_filename(char *buffer, const char *path,
                               const char *prefix, const char *suffix,
                               bool unique_time);
#endif /* CONFIG_RTC */

/* Ask the user if they really want to erase the current dynamic playlist
 * returns true if the playlist should be replaced */
bool warn_on_pl_erase(void);

/* Read (up to) a line of text from fd into buffer and return number of bytes
 * read (which may be larger than the number of bytes stored in buffer). If 
 * an error occurs, -1 is returned (and buffer contains whatever could be 
 * read). A line is terminated by a LF char. Neither LF nor CR chars are 
 * stored in buffer.
 */
int read_line(int fd, char* buffer, int buffer_size);
int fast_readline(int fd, char *buf, int buf_size, void *parameters,
                  int (*callback)(int n, const char *buf, void *parameters));

#ifdef HAVE_LCD_BITMAP
/* Save a .BMP file containing the current screen contents. */
void screen_dump(void);
void screen_dump_set_hook(void (*hook)(int fh));
#endif

bool settings_parseline(char* line, char** name, char** value);
long default_event_handler_ex(long event, void (*callback)(void *), void *parameter);
long default_event_handler(long event);
bool list_stop_handler(void);
void car_adapter_mode_init(void);
extern int show_logo(void);

#if CONFIG_CODEC == SWCODEC
/* Return current ReplayGain mode a file should have (REPLAYGAIN_TRACK or
 * REPLAYGAIN_ALBUM) if ReplayGain processing is enabled, or -1 if no
 * information present.
 */
int get_replaygain_mode(bool have_track_gain, bool have_album_gain);
#endif

int open_utf8(const char* pathname, int flags);

#ifdef BOOTFILE
#if !defined(USB_NONE) && !defined(USB_IPODSTYLE)
void check_bootfile(bool do_rolo);
#endif
#endif

/* check range, set volume and save settings */
void setvol(void);

#ifdef HAVE_LCD_COLOR
int hex_to_rgb(const char* hex, int* color);
#endif

char* strrsplt(char* str, int c);
char* skip_whitespace(char* const str);
bool file_exists(const char *file);
bool dir_exists(const char *path);

/*
 * removes the extension of filename (if it doesn't start with a .)
 * puts the result in buffer
 */
char *strip_extension(char* buffer, int buffer_size, const char *filename);

/* A simplified scanf */
/*
 * Checks whether the value at position 'position' was really read
 * during a call to 'parse_list'
 *   - position: 0-based number of the value
 *   - valid_vals: value after the call to 'parse_list'
 */
#define LIST_VALUE_PARSED(setvals, position) ((setvals)&(1<<(position)))
const char* parse_list(const char *fmt, uint32_t *set_vals,
                       const char sep, const char* str, ...);

#endif /* MISC_H */

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
#include "screen_access.h"

extern const unsigned char * const byte_units[];
extern const unsigned char * const * const kibyte_units;
extern const unsigned char * const unit_strings_core[];
/* Format a large-range value for output, using the appropriate unit so that
 * the displayed value is in the range 1 <= display < 1000 (1024 for "binary"
 * units) if possible, and 3 significant digits are shown. If a buffer is
 * given, the result is snprintf()'d into that buffer, otherwise the result is
 * voiced.*/
char *output_dyn_value(char *buf,
                       int buf_size,
                       int value,
                       const unsigned char * const *units,
                       unsigned int unit_count,
                       bool binary_scale);


/* format_time_auto */
enum e_fmt_time_auto_idx
{
    UNIT_IDX_HR = 0,
    UNIT_IDX_MIN,
    UNIT_IDX_SEC,
    UNIT_IDX_MS,
    UNIT_IDX_TIME_COUNT,
};
#define UNIT_IDX_MASK       0x01FFU /*Return only Unit_IDX*/
#define UNIT_TRIM_ZERO      0x0200U /*Don't show leading zero on max_idx*/
#define UNIT_LOCK_HR        0x0400U /*Don't Auto Range below this field*/
#define UNIT_LOCK_MIN       0x0800U /*Don't Auto Range below this field*/
#define UNIT_LOCK_SEC       0x1000U /*Don't Auto Range below this field*/

/*  time_split_units()
    split time values depending on base unit
    unit_idx: UNIT_HOUR, UNIT_MIN, UNIT_SEC, UNIT_MS
    abs_value: absolute time value
    units_in: array of unsigned ints with IDX_TIME_COUNT fields
*/
unsigned int time_split_units(int unit_idx, unsigned long abs_val,
                        unsigned long (*units_in)[UNIT_IDX_TIME_COUNT]);

/* format_time_auto - return an auto ranged time string;
   buffer:  needs to be at least 25 characters for full range

   unit_idx: specifies lowest or base index of the value
   add | UNIT_LOCK_ to keep place holder of units that would normally be
   discarded.. For instance, UNIT_LOCK_HR would keep the hours place, ex: string
   00:10:10 (0 HRS 10 MINS 10 SECONDS) normally it would return as 10:10
   add | UNIT_TRIM_ZERO to supress leading zero on the largest unit

   value: should be passed in the same form as unit_idx

   supress_unit: may be set to true and in this case the
   hr, min, sec, ms identifiers will be left off the resulting string but
   since right to left languages are handled it is advisable to leave units
   as an indication of the text direction
*/
const char *format_time_auto(char *buffer, int buf_len, long value,
                                  int unit_idx, bool supress_unit);

/* Format time into buf.
 *
 * buf      - buffer to format to.
 * buf_size - size of buffer.
 * t        - time to format, in milliseconds.
 */
void format_time(char* buf, int buf_size, long t);

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
                  int (*callback)(int n, char *buf, void *parameters));

bool settings_parseline(char* line, char** name, char** value);
long default_event_handler_ex(long event, void (*callback)(void *), void *parameter);
long default_event_handler(long event);
bool list_stop_handler(void);
void car_adapter_mode_init(void) INIT_ATTR;
extern int show_logo(void);

/* Unicode byte order mark sequences and lengths */
#define BOM_UTF_8 "\xef\xbb\xbf"
#define BOM_UTF_8_SIZE 3
#define BOM_UTF_16_LE "\xff\xfe"
#define BOM_UTF_16_BE "\xfe\xff"
#define BOM_UTF_16_SIZE 2

int split_string(char *str, const char needle, char *vector[], int vector_length);
int open_utf8(const char* pathname, int flags);
int string_option(const char *option, const char *const oplist[], bool ignore_case);

#ifdef BOOTFILE
#if !defined(USB_NONE) && !defined(USB_HANDLED_BY_OF) \
        || defined(HAVE_HOTSWAP_STORAGE_AS_MAIN)
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

/*
 * removes the extension of filename (if it doesn't start with a .)
 * puts the result in buffer
 */
char *strip_extension(char* buffer, int buffer_size, const char *filename);

bool parse_color(enum screen_type screen, char *text, int *value);

/* only used in USB HID and set_time screen */
#if defined(USB_ENABLE_HID) || (CONFIG_RTC != 0)
int clamp_value_wrap(int value, int max, int min);
#endif

enum current_activity {
    ACTIVITY_UNKNOWN = 0,
    ACTIVITY_MAINMENU,
    ACTIVITY_WPS,
    ACTIVITY_RECORDING,
    ACTIVITY_FM,
    ACTIVITY_PLAYLISTVIEWER,
    ACTIVITY_SETTINGS,
    ACTIVITY_FILEBROWSER,
    ACTIVITY_DATABASEBROWSER,
    ACTIVITY_PLUGINBROWSER,
    ACTIVITY_QUICKSCREEN,
    ACTIVITY_PITCHSCREEN,
    ACTIVITY_OPTIONSELECT,
    ACTIVITY_PLAYLISTBROWSER,
    ACTIVITY_PLUGIN,
    ACTIVITY_CONTEXTMENU,
    ACTIVITY_SYSTEMSCREEN,
    ACTIVITY_TIMEDATESCREEN,
    ACTIVITY_BOOKMARKSLIST,
    ACTIVITY_SHORTCUTSMENU,
    ACTIVITY_ID3SCREEN,
    ACTIVITY_USBSCREEN
};

void beep_play(unsigned int frequency, unsigned int duration,
               unsigned int amplitude);

enum system_sound
{
    SOUND_KEYCLICK = 0,
    SOUND_TRACK_SKIP,
    SOUND_TRACK_NO_MORE,
    SOUND_LIST_EDGE_BEEP_WRAP,
    SOUND_LIST_EDGE_BEEP_NOWRAP,
};

/* Play a standard sound */
void system_sound_play(enum system_sound sound);

typedef bool (*keyclick_callback)(int action, void* data);
void keyclick_set_callback(keyclick_callback cb, void* data);
/* Produce keyclick based upon button and global settings */
void keyclick_click(bool rawbutton, int action);

/* Return current ReplayGain mode a file should have (REPLAYGAIN_TRACK or
 * REPLAYGAIN_ALBUM) if ReplayGain processing is enabled, or -1 if no
 * information present.
 */
struct mp3entry;
int id3_get_replaygain_mode(const struct mp3entry *id3);
void replaygain_update(void);

void push_current_activity(enum current_activity screen);
void pop_current_activity(void);
enum current_activity get_current_activity(void);

/* format a sound value like: -1.05 dB */
int format_sound_value(char *buf, size_t len, int snd, int val);

#endif /* MISC_H */

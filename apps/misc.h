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
extern const unsigned char * const * const kbyte_units;

/* Format a large-range value for output, using the appropriate unit so that
 * the displayed value is in the range 1 <= display < 1000 (1024 for "binary"
 * units) if possible, and 3 significant digits are shown. If a buffer is
 * given, the result is snprintf()'d into that buffer, otherwise the result is
 * voiced.*/
char *output_dyn_value(char *buf, int buf_size, int value,
                      const unsigned char * const *units, bool bin_scale);

extern const unsigned char * const unit_strings_core[];
/* format_time_auto */
#define UNIT_IDX_HR    0x0U
#define UNIT_IDX_MIN   0x1U
#define UNIT_IDX_SEC   0x2U
#define UNIT_IDX_MS    0x3U
#define IDX_TIME_COUNT 0x4U
#define UNIT_IDX_MASK  0x1FFU/*Return only Unit_IDX*/
#define UNIT_LOCK_MASK 0x7E00/*Return only Unit_LOCK*/
#define UNIT_LOCK_HR   0x200U /*Don't Auto Range below this field*/
#define UNIT_LOCK_MIN  0x400U /*Don't Auto Range below this field*/
#define UNIT_LOCK_SEC  0x800U /*Don't Auto Range below this field*/
#define UNIT_SHORT_MIN 0x1000 /*legacy behaviour for minute field*/

/* format_time_auto - return an auto ranged time string,

   unit_idx: specifies lowest or base index of the value,

   VALUE should be passed in the same form as unit_idx
   if unit_idx is HOURS then the value passed should be in HOURS!
   if unit_idx is MS then the value passed should be in MILLISECONDS!
   same for SEC & MIN

   add | UNIT_LOCK_ to keep place holder of units that would normally be
   discarded.. For instance, UNIT_LOCK_HR would keep the hours place, ex: string
   00:10:10 (0 HRS 10 MINS 10 SECONDS) normally it would return as 10:10

   supress_unit may be set to true and in this case the
   hr, min, sec, ms identifiers will be left off the resulting string but
   since right to left languages are handled it is advisable to leave units
   as an indication of the text direction

   idx_pos[2]: (if !NULL) [0] specifies an index of interest,
   the offset in the string for that index will be returned
   in field [0] offset, field [1] will be the length
   Ex: in a string 12:34:56.78 if you pass the idx_pos UNIT_IDX_MIN then you
   will be given the offset(3) of 34 and a length of 2
*/

const char *format_time_auto(char *buffer, int buf_len, const int value,
                                  int unit_idx, bool supress_unit,
                                  unsigned int (*idx_pos)[2]);

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

#ifdef HAVE_LCD_BITMAP
bool parse_color(enum screen_type screen, char *text, int *value);

/* only used in USB HID and set_time screen */
#if defined(USB_ENABLE_HID) || (CONFIG_RTC != 0)
int clamp_value_wrap(int value, int max, int min);
#endif
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

#if CONFIG_CODEC == SWCODEC
void beep_play(unsigned int frequency, unsigned int duration,
               unsigned int amplitude);

enum system_sound
{
    SOUND_KEYCLICK = 0,
    SOUND_TRACK_SKIP,
    SOUND_TRACK_NO_MORE,
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
#else
static inline void replaygain_update(void) {}
#endif /* CONFIG_CODEC == SWCODEC */

void push_current_activity(enum current_activity screen);
void pop_current_activity(void);
enum current_activity get_current_activity(void);


#endif /* MISC_H */

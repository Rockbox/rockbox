/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Robert Bieber
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

#ifndef TAG_TABLE_H
#define TAG_TABLE_H

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_TAG_LENGTH 4 /* includes the \0 */
#define MAX_TAG_PARAMS 12

#define NOBREAK 0x1 /* Flag to tell the renderer not to insert a line break */
#define FEATURE_TAG 0x2 /* Parse time conditional for feature checks (e.g HAVE_RTC) */

/* constants used in line_type and as refresh_mode for wps_refresh */
#define SKIN_REFRESH_SHIFT           16
#define SKIN_REFRESH_STATIC          (1u<<SKIN_REFRESH_SHIFT)  /* line doesn't change over time */
#define SKIN_REFRESH_DYNAMIC         (1u<<(SKIN_REFRESH_SHIFT+1))  /* line may change (e.g. time flag) */
#define SKIN_REFRESH_SCROLL          (1u<<(SKIN_REFRESH_SHIFT+2))  /* line scrolls */
#define SKIN_REFRESH_PLAYER_PROGRESS (1u<<(SKIN_REFRESH_SHIFT+3))  /* line contains a progress bar */
#define SKIN_REFRESH_PEAK_METER      (1u<<(SKIN_REFRESH_SHIFT+4))  /* line contains a peak meter */
#define SKIN_REFRESH_STATUSBAR       (1u<<(SKIN_REFRESH_SHIFT+5))  /* refresh statusbar */
#define SKIN_RTC_REFRESH             (1u<<(SKIN_REFRESH_SHIFT+6))  /* refresh rtc, convert at parse time */
#define SKIN_REFRESH_ALL             (0xffffu<<SKIN_REFRESH_SHIFT)   /* to refresh all line types */

/* to refresh only those lines that change over time */
#define SKIN_REFRESH_NON_STATIC (SKIN_REFRESH_DYNAMIC| \
                                 SKIN_REFRESH_PLAYER_PROGRESS| \
                                 SKIN_REFRESH_PEAK_METER)

enum skin_token_type {
    
    SKIN_TOKEN_NO_TOKEN,
    SKIN_TOKEN_UNKNOWN,

    /* Markers */
    SKIN_TOKEN_CHARACTER,
    SKIN_TOKEN_STRING,
    SKIN_TOKEN_TRANSLATEDSTRING,

    /* Alignment */
    SKIN_TOKEN_ALIGN_LEFT,
    SKIN_TOKEN_ALIGN_LEFT_RTL,
    SKIN_TOKEN_ALIGN_CENTER,
    SKIN_TOKEN_ALIGN_RIGHT,
    SKIN_TOKEN_ALIGN_RIGHT_RTL,
    SKIN_TOKEN_ALIGN_LANGDIRECTION,
    

    /* Sublines */
    SKIN_TOKEN_SUBLINE_TIMEOUT,
    SKIN_TOKEN_SUBLINE_SCROLL,
    
    /* Conditional */
    SKIN_TOKEN_LOGICAL_IF,
    SKIN_TOKEN_LOGICAL_AND,
    SKIN_TOKEN_LOGICAL_OR,
    SKIN_TOKEN_CONDITIONAL,
    SKIN_TOKEN_CONDITIONAL_START,
    SKIN_TOKEN_CONDITIONAL_OPTION,
    SKIN_TOKEN_CONDITIONAL_END,

    /* Viewport display */
    SKIN_TOKEN_VIEWPORT_LOAD,
    SKIN_TOKEN_VIEWPORT_CONDITIONAL,
    SKIN_TOKEN_VIEWPORT_ENABLE,
    SKIN_TOKEN_VIEWPORT_CUSTOMLIST,
    SKIN_TOKEN_UIVIEWPORT_ENABLE,
    SKIN_TOKEN_UIVIEWPORT_LOAD,
    SKIN_TOKEN_VIEWPORT_FGCOLOUR,
    SKIN_TOKEN_VIEWPORT_BGCOLOUR,
    SKIN_TOKEN_VIEWPORT_TEXTSTYLE,
    SKIN_TOKEN_VIEWPORT_GRADIENT_SETUP,
    SKIN_TOKEN_VIEWPORT_DRAWONBG,

    /* Battery */
    SKIN_TOKEN_BATTERY_PERCENT,
    SKIN_TOKEN_BATTERY_PERCENTBAR,
    SKIN_TOKEN_BATTERY_VOLTS,
    SKIN_TOKEN_BATTERY_TIME,
    SKIN_TOKEN_BATTERY_CHARGER_CONNECTED,
    SKIN_TOKEN_BATTERY_CHARGING,
    SKIN_TOKEN_BATTERY_SLEEPTIME,
    SKIN_TOKEN_USB_POWERED,

    /* Sound */
    SKIN_TOKEN_SOUND_PITCH,
    SKIN_TOKEN_SOUND_SPEED,
    SKIN_TOKEN_REPLAYGAIN,
    SKIN_TOKEN_CROSSFADE,

    /* Time */
    SKIN_TOKEN_RTC_PRESENT,

    /* The begin/end values allow us to know if a token is an RTC one.
       New RTC tokens should be added between the markers. */

    SKIN_TOKENS_RTC_BEGIN, /* just the start marker, not an actual token */

    SKIN_TOKEN_RTC_DAY_OF_MONTH,
    SKIN_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED,
    SKIN_TOKEN_RTC_12HOUR_CFG,
    SKIN_TOKEN_RTC_HOUR_24_ZERO_PADDED,
    SKIN_TOKEN_RTC_HOUR_24,
    SKIN_TOKEN_RTC_HOUR_12_ZERO_PADDED,
    SKIN_TOKEN_RTC_HOUR_12,
    SKIN_TOKEN_RTC_MONTH,
    SKIN_TOKEN_RTC_MINUTE,
    SKIN_TOKEN_RTC_SECOND,
    SKIN_TOKEN_RTC_YEAR_2_DIGITS,
    SKIN_TOKEN_RTC_YEAR_4_DIGITS,
    SKIN_TOKEN_RTC_AM_PM_UPPER,
    SKIN_TOKEN_RTC_AM_PM_LOWER,
    SKIN_TOKEN_RTC_WEEKDAY_NAME,
    SKIN_TOKEN_RTC_MONTH_NAME,
    SKIN_TOKEN_RTC_DAY_OF_WEEK_START_MON,
    SKIN_TOKEN_RTC_DAY_OF_WEEK_START_SUN,

    SKIN_TOKENS_RTC_END,     /* just the end marker, not an actual token */

    /* Database */
    SKIN_TOKEN_DATABASE_PLAYCOUNT,
    SKIN_TOKEN_DATABASE_RATING,
    SKIN_TOKEN_DATABASE_AUTOSCORE,

    /* File */
    SKIN_TOKEN_FILE_BITRATE,
    SKIN_TOKEN_FILE_CODEC,
    SKIN_TOKEN_FILE_FREQUENCY,
    SKIN_TOKEN_FILE_FREQUENCY_KHZ,
    SKIN_TOKEN_FILE_NAME,
    SKIN_TOKEN_FILE_NAME_WITH_EXTENSION,
    SKIN_TOKEN_FILE_PATH,
    SKIN_TOKEN_FILE_SIZE,
    SKIN_TOKEN_FILE_VBR,
    SKIN_TOKEN_FILE_DIRECTORY,

    /* Image */
    SKIN_TOKEN_IMAGE_BACKDROP,
    SKIN_TOKEN_IMAGE_PROGRESS_BAR,
    SKIN_TOKEN_IMAGE_PRELOAD,
    SKIN_TOKEN_IMAGE_PRELOAD_DISPLAY,
    SKIN_TOKEN_IMAGE_DISPLAY,
    SKIN_TOKEN_IMAGE_DISPLAY_LISTICON,
    
    /* Albumart */
    SKIN_TOKEN_ALBUMART_LOAD,
    SKIN_TOKEN_ALBUMART_DISPLAY,
    SKIN_TOKEN_ALBUMART_FOUND,

    /* Metadata */
    SKIN_TOKEN_METADATA_ARTIST,
    SKIN_TOKEN_METADATA_COMPOSER,
    SKIN_TOKEN_METADATA_ALBUM_ARTIST,
    SKIN_TOKEN_METADATA_GROUPING,
    SKIN_TOKEN_METADATA_ALBUM,
    SKIN_TOKEN_METADATA_GENRE,
    SKIN_TOKEN_METADATA_DISC_NUMBER,
    SKIN_TOKEN_METADATA_TRACK_NUMBER,
    SKIN_TOKEN_METADATA_TRACK_TITLE,
    SKIN_TOKEN_METADATA_VERSION,
    SKIN_TOKEN_METADATA_YEAR,
    SKIN_TOKEN_METADATA_COMMENT,

    /* Mode */
    SKIN_TOKEN_REPEAT_MODE,
    SKIN_TOKEN_PLAYBACK_STATUS,
    /* Progressbar */
    SKIN_TOKEN_PROGRESSBAR,
    SKIN_TOKEN_PLAYER_PROGRESSBAR,
    /* Peakmeter */
    SKIN_TOKEN_PEAKMETER,
    SKIN_TOKEN_PEAKMETER_LEFT,
    SKIN_TOKEN_PEAKMETER_LEFTBAR,
    SKIN_TOKEN_PEAKMETER_RIGHT,
    SKIN_TOKEN_PEAKMETER_RIGHTBAR,

    /* Current track */
    SKIN_TOKEN_TRACK_ELAPSED_PERCENT,
    SKIN_TOKEN_TRACK_TIME_ELAPSED,
    SKIN_TOKEN_TRACK_TIME_REMAINING,
    SKIN_TOKEN_TRACK_LENGTH,
    SKIN_TOKEN_TRACK_STARTING,
    SKIN_TOKEN_TRACK_ENDING,

    /* Playlist */
    SKIN_TOKEN_PLAYLIST_ENTRIES,
    SKIN_TOKEN_PLAYLIST_NAME,
    SKIN_TOKEN_PLAYLIST_POSITION,
    SKIN_TOKEN_PLAYLIST_SHUFFLE,


    SKIN_TOKEN_ENABLE_THEME,
    SKIN_TOKEN_DISABLE_THEME,
    SKIN_TOKEN_DRAW_INBUILTBAR,
    SKIN_TOKEN_LIST_TITLE_TEXT,
    SKIN_TOKEN_LIST_TITLE_ICON,
    SKIN_TOKEN_LIST_ITEM_CFG,
    SKIN_TOKEN_LIST_SELECTED_ITEM_CFG,
    SKIN_TOKEN_LIST_ITEM_IS_SELECTED,
    SKIN_TOKEN_LIST_ITEM_TEXT,
    SKIN_TOKEN_LIST_ITEM_ROW,
    SKIN_TOKEN_LIST_ITEM_COLUMN,
    SKIN_TOKEN_LIST_ITEM_NUMBER,
    SKIN_TOKEN_LIST_ITEM_ICON,
    SKIN_TOKEN_LIST_NEEDS_SCROLLBAR,
    SKIN_TOKEN_LIST_SCROLLBAR,
    
    SKIN_TOKEN_LOAD_FONT,
    
    /* buttons */
    SKIN_TOKEN_BUTTON_VOLUME,
    SKIN_TOKEN_LASTTOUCH,
    SKIN_TOKEN_TOUCHREGION,
    SKIN_TOKEN_HAVE_TOUCH,
    
    /* Virtual LED */
    SKIN_TOKEN_VLED_HDD,
    /* Volume level */
    SKIN_TOKEN_VOLUME,
    SKIN_TOKEN_VOLUMEBAR,
    /* hold */
    SKIN_TOKEN_MAIN_HOLD,
    SKIN_TOKEN_REMOTE_HOLD,

    /* Setting option */
    SKIN_TOKEN_SETTING,
    SKIN_TOKEN_CURRENT_SCREEN,
    SKIN_TOKEN_LANG_IS_RTL,
    
    /* Recording Tokens */
    SKIN_TOKEN_HAVE_RECORDING,
    SKIN_TOKEN_IS_RECORDING,
    SKIN_TOKEN_REC_FREQ,
    SKIN_TOKEN_REC_ENCODER,
    SKIN_TOKEN_REC_BITRATE, /* SWCODEC: MP3 bitrate, HWCODEC: MP3 "quality" */
    SKIN_TOKEN_REC_MONO,
    SKIN_TOKEN_REC_SECONDS,
    SKIN_TOKEN_REC_MINUTES,
    SKIN_TOKEN_REC_HOURS,
    
    
    /* Radio Tokens */
    SKIN_TOKEN_HAVE_TUNER,
    SKIN_TOKEN_TUNER_TUNED,
    SKIN_TOKEN_TUNER_SCANMODE,
    SKIN_TOKEN_TUNER_STEREO,
    SKIN_TOKEN_TUNER_MINFREQ, /* changes based on "region" */
    SKIN_TOKEN_TUNER_MAXFREQ, /* changes based on "region" */
    SKIN_TOKEN_TUNER_CURFREQ,
    SKIN_TOKEN_TUNER_RSSI,
    SKIN_TOKEN_TUNER_RSSI_MIN,
    SKIN_TOKEN_TUNER_RSSI_MAX,
    SKIN_TOKEN_TUNER_RSSI_BAR,
    SKIN_TOKEN_PRESET_ID, /* "id" of this preset.. really the array element number */
    SKIN_TOKEN_PRESET_NAME,
    SKIN_TOKEN_PRESET_FREQ,
    SKIN_TOKEN_PRESET_COUNT,
    /* RDS tokens */
    SKIN_TOKEN_HAVE_RDS,
    SKIN_TOKEN_RDS_NAME,
    SKIN_TOKEN_RDS_TEXT,

    /* Skin variables */
    SKIN_TOKEN_VAR_SET,
    SKIN_TOKEN_VAR_GETVAL,
    SKIN_TOKEN_VAR_TIMEOUT,

    SKIN_TOKEN_SUBSTRING,

    SKIN_TOKEN_DRAWRECTANGLE,
};

/*
 * Struct for tag parsing information
 * name   - The name of the tag, i.e. V for %V
 * params - A string specifying all of the tags parameters, each
 *          character representing a single parameter.  Valid
 *          characters for parameters are:
 *             I - Required integer
 *             i - Nullable integer
 *             D - Required decimal 
 *             d - Nullable decimal
 *                  Decimals are stored as (whole*10)+part
 *             S - Required string
 *             s - Nullable string
 *             F - Required file name
 *             f - Nullable file name
 *             C - Required skin code
 *             T - Required single skin tag
 *             * - Any amonut of the previous tag (or group if after a []
 *             \n - causes the parser to eat everything up to and including the \n
 *                  MUST be the last character of the prams string
 *          Any nullable parameter may be replaced in the WPS file
 *          with a '-'.  To specify that parameters may be left off
 *          altogether, place a '|' in the parameter string.  For
 *          instance, with the parameter string...
 *             Ii|Ss
 *          one integer must be specified, one integer can be
 *          specified or set to default with '-', and the user can
 *          stop providing parameters at any time after that.
 *          To specify multiple instances of the same type, put a 
 *          number before the character.  For instance, the string...
 *             2s
 *          will specify two strings.  A ? at the beginning of the
 *          string will specify that you may choose to omit all arguments
 * 
 *          You may also group param types in [] which will tell the parser to 
 *          accept any *one* of those types for that param. i.e [IT] will
 *          accept either an integer or tag type. [ITs] will also accept a string or -
 *
 */
struct tag_info
{
    enum skin_token_type type;
    char* name;
    char* params;
    int flags;
};

/* 
 * Finds a tag by name and returns its parameter list, or an empty
 * string if the tag is not found in the table
 */
const struct tag_info* find_tag(const char* name);

/*
 * Determines whether a character is legal to escape or not.  If 
 * lookup is not found in the legal escape characters string, returns
 * false, otherwise returns true
 */
int find_escape_character(char lookup);

#ifdef __cplusplus
}
#endif

#endif /* TAG_TABLE_H */

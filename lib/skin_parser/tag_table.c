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

#include "tag_table.h"

#include <string.h>
#define BAR_PARAMS "?iiii|s*"
/* The tag definition table */
static const struct tag_info legal_tags[] = 
{
    { SKIN_TOKEN_ALIGN_CENTER,          "ac", "", 0 },
    { SKIN_TOKEN_ALIGN_LEFT,            "al", "", 0 },
    { SKIN_TOKEN_ALIGN_LEFT_RTL,        "aL", "", 0 },
    { SKIN_TOKEN_ALIGN_RIGHT,           "ar", "", 0 },
    { SKIN_TOKEN_ALIGN_RIGHT_RTL,       "aR", "", 0 },
    { SKIN_TOKEN_ALIGN_LANGDIRECTION,   "ax", "", 0 },
    
    { SKIN_TOKEN_LOGICAL_IF,            "if", "TS[ITS]|D", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_LOGICAL_AND,           "and", "T*", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_LOGICAL_OR,            "or", "T*", SKIN_REFRESH_DYNAMIC },
    
    { SKIN_TOKEN_BATTERY_PERCENT,       "bl" , BAR_PARAMS, SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_BATTERY_VOLTS,         "bv", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_BATTERY_TIME,          "bt", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_BATTERY_SLEEPTIME,     "bs", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_BATTERY_CHARGING,      "bc", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_BATTERY_CHARGER_CONNECTED, "bp", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_USB_POWERED,           "bu", "", SKIN_REFRESH_DYNAMIC },
    
    
    { SKIN_TOKEN_RTC_PRESENT,           "cc", "", FEATURE_TAG },
    { SKIN_TOKEN_RTC_DAY_OF_MONTH,      "cd", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED, "ce", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_12HOUR_CFG,        "cf", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_HOUR_24_ZERO_PADDED, "cH", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_HOUR_24,           "ck", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_HOUR_12_ZERO_PADDED, "cI", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_HOUR_12,           "cl", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_MONTH,             "cm", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_MINUTE,            "cM", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_SECOND,            "cS", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_YEAR_2_DIGITS,     "cy", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_YEAR_4_DIGITS,     "cY", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_AM_PM_UPPER,       "cP", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_AM_PM_LOWER,       "cp", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_WEEKDAY_NAME,      "ca", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_MONTH_NAME,        "cb", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_DAY_OF_WEEK_START_MON, "cu", "", SKIN_RTC_REFRESH },
    { SKIN_TOKEN_RTC_DAY_OF_WEEK_START_SUN, "cw", "", SKIN_RTC_REFRESH },
        
    { SKIN_TOKEN_FILE_BITRATE,          "fb", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_CODEC,            "fc", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_FREQUENCY,        "ff", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_FREQUENCY_KHZ,    "fk", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_NAME_WITH_EXTENSION,  "fm", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_NAME,             "fn", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_PATH,             "fp", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_SIZE,             "fs", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_VBR,              "fv", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_DIRECTORY,        "d"  , "I", SKIN_REFRESH_STATIC },
    
    { SKIN_TOKEN_FILE_BITRATE,          "Fb", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_CODEC,            "Fc", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_FREQUENCY,        "Ff", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_FREQUENCY_KHZ,    "Fk", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_NAME_WITH_EXTENSION,  "Fm", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_NAME,             "Fn", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_PATH,             "Fp", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_SIZE,             "Fs", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_VBR,              "Fv", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_FILE_DIRECTORY,        "D"  , "I", SKIN_REFRESH_STATIC },
    
    
    { SKIN_TOKEN_METADATA_ARTIST,       "ia", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_COMPOSER,     "ic", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_ALBUM,        "id", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_ALBUM_ARTIST, "iA", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_GROUPING,     "iG", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_GENRE,        "ig", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_DISC_NUMBER,  "ik", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_TRACK_NUMBER, "in", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_TRACK_TITLE,  "it", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_VERSION,      "iv", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_YEAR,         "iy", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_COMMENT,      "iC", "", SKIN_REFRESH_STATIC },
    
    { SKIN_TOKEN_METADATA_ARTIST,       "Ia", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_COMPOSER,     "Ic", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_ALBUM,        "Id", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_ALBUM_ARTIST, "IA", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_GROUPING,     "IG", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_GENRE,        "Ig", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_DISC_NUMBER,  "Ik", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_TRACK_NUMBER, "In", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_TRACK_TITLE,  "It", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_VERSION,      "Iv", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_YEAR,         "Iy", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_METADATA_COMMENT,      "IC", "", SKIN_REFRESH_STATIC },
    
    { SKIN_TOKEN_SOUND_PITCH,           "Sp", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_SOUND_SPEED,           "Ss", "", SKIN_REFRESH_DYNAMIC },
    
    { SKIN_TOKEN_VLED_HDD,              "lh", "", SKIN_REFRESH_DYNAMIC },
    
    { SKIN_TOKEN_MAIN_HOLD,             "mh", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_REMOTE_HOLD,           "mr", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_REPEAT_MODE,           "mm", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_PLAYBACK_STATUS,       "mp", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_BUTTON_VOLUME,         "mv", "|D", SKIN_REFRESH_DYNAMIC },
    
    { SKIN_TOKEN_PEAKMETER,             "pm", "", SKIN_REFRESH_PEAK_METER },
    { SKIN_TOKEN_PEAKMETER_LEFT,        "pL", BAR_PARAMS, SKIN_REFRESH_PEAK_METER },
    { SKIN_TOKEN_PEAKMETER_RIGHT,       "pR", BAR_PARAMS, SKIN_REFRESH_PEAK_METER },
    
    { SKIN_TOKEN_PLAYER_PROGRESSBAR,    "pf", "", SKIN_REFRESH_DYNAMIC|SKIN_REFRESH_PLAYER_PROGRESS },
    { SKIN_TOKEN_PROGRESSBAR,           "pb" , BAR_PARAMS, SKIN_REFRESH_PLAYER_PROGRESS },
    { SKIN_TOKEN_VOLUME,                "pv" , BAR_PARAMS, SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TRACK_ELAPSED_PERCENT, "px", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TRACK_TIME_ELAPSED,    "pc", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TRACK_TIME_REMAINING,  "pr", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TRACK_LENGTH,          "pt", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_TRACK_STARTING,        "pS" , "|D", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TRACK_ENDING,          "pE" , "|D", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_PLAYLIST_POSITION,     "pp", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_PLAYLIST_ENTRIES,      "pe", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_PLAYLIST_NAME,         "pn", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_PLAYLIST_SHUFFLE,      "ps", "", SKIN_REFRESH_DYNAMIC },
    
    { SKIN_TOKEN_DATABASE_PLAYCOUNT,    "rp", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_DATABASE_RATING,       "rr", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_DATABASE_AUTOSCORE,    "ra", "", SKIN_REFRESH_DYNAMIC },
    
    { SKIN_TOKEN_REPLAYGAIN,            "rg", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_CROSSFADE,             "xf", "", SKIN_REFRESH_DYNAMIC },
    
    { SKIN_TOKEN_HAVE_TUNER,            "tp", "", FEATURE_TAG },
    { SKIN_TOKEN_TUNER_TUNED,           "tt", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TUNER_SCANMODE,        "tm", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TUNER_STEREO,          "ts", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TUNER_MINFREQ,         "ta", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_TUNER_MAXFREQ,         "tb", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_TUNER_CURFREQ,         "tf", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TUNER_RSSI,            "tr", BAR_PARAMS, SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TUNER_RSSI_MIN,        "tl", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_TUNER_RSSI_MAX,        "th", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_PRESET_ID,             "Ti", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_PRESET_NAME,           "Tn", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_PRESET_FREQ,           "Tf", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_PRESET_COUNT,          "Tc", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_HAVE_RDS,              "tx", "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_RDS_NAME,              "ty", "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_RDS_TEXT,              "tz", "", SKIN_REFRESH_DYNAMIC },

    { SKIN_TOKEN_SUBLINE_SCROLL,        "s", "", SKIN_REFRESH_SCROLL },
    { SKIN_TOKEN_SUBLINE_TIMEOUT,       "t"  , "D", 0 },

    { SKIN_TOKEN_ENABLE_THEME,          "we", "", 0|NOBREAK },
    { SKIN_TOKEN_DISABLE_THEME,         "wd", "", 0|NOBREAK },
    { SKIN_TOKEN_DRAW_INBUILTBAR,       "wi", "", SKIN_REFRESH_STATIC|NOBREAK },
    
    { SKIN_TOKEN_IMAGE_PRELOAD,         "xl", "SF|III", 0|NOBREAK },
    { SKIN_TOKEN_IMAGE_PRELOAD_DISPLAY, "xd", "S|[IT]I", 0 },
    { SKIN_TOKEN_IMAGE_DISPLAY,         "x", "SF|II", SKIN_REFRESH_STATIC|NOBREAK },
    
    { SKIN_TOKEN_LOAD_FONT,             "Fl" , "IF|I", 0|NOBREAK },
    { SKIN_TOKEN_ALBUMART_LOAD,         "Cl" , "IIII|ss", 0|NOBREAK },
    { SKIN_TOKEN_ALBUMART_DISPLAY,      "Cd" , "", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_ALBUMART_FOUND,        "C" , "", SKIN_REFRESH_STATIC },
    
    { SKIN_TOKEN_VIEWPORT_ENABLE,       "Vd" , "S", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_UIVIEWPORT_ENABLE,     "VI" , "S", SKIN_REFRESH_STATIC },
    
    { SKIN_TOKEN_VIEWPORT_CUSTOMLIST,   "Vp" , "IC", SKIN_REFRESH_DYNAMIC|NOBREAK },
    { SKIN_TOKEN_LIST_TITLE_TEXT,       "Lt" , "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_LIST_ITEM_TEXT,        "LT", "|IS",  SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_LIST_ITEM_ROW,         "LR", "",  SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_LIST_ITEM_COLUMN,      "LC", "",  SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_LIST_ITEM_NUMBER,      "LN", "",  SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_LIST_TITLE_ICON,       "Li" , "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_LIST_ITEM_ICON,        "LI", "|IS",  SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_LIST_ITEM_CFG,         "Lb" , "Sii|S", SKIN_REFRESH_DYNAMIC},
    { SKIN_TOKEN_LIST_ITEM_IS_SELECTED, "Lc" , "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_LIST_NEEDS_SCROLLBAR,  "LB", BAR_PARAMS, SKIN_REFRESH_DYNAMIC },
    
    { SKIN_TOKEN_VIEWPORT_FGCOLOUR,       "Vf" , "s", SKIN_REFRESH_STATIC|NOBREAK },
    { SKIN_TOKEN_VIEWPORT_BGCOLOUR,       "Vb" , "s", SKIN_REFRESH_STATIC|NOBREAK },
    { SKIN_TOKEN_VIEWPORT_TEXTSTYLE,      "Vs" , "S|s", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_VIEWPORT_GRADIENT_SETUP, "Vg" , "SS|s", SKIN_REFRESH_STATIC|NOBREAK },
    { SKIN_TOKEN_VIEWPORT_DRAWONBG,       "VB" , "", SKIN_REFRESH_STATIC|NOBREAK },
    
    { SKIN_TOKEN_VIEWPORT_CONDITIONAL,  "Vl" , "SIIiii", 0 },
    { SKIN_TOKEN_UIVIEWPORT_LOAD,       "Vi" , "sIIiii", 0 },
    { SKIN_TOKEN_VIEWPORT_LOAD,         "V"  , "IIiii", 0 },
    
    { SKIN_TOKEN_IMAGE_BACKDROP,        "X"  , "f", SKIN_REFRESH_STATIC|NOBREAK },
    
    { SKIN_TOKEN_SETTING,               "St" , "S", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TRANSLATEDSTRING,      "Sx" , "S", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_LANG_IS_RTL,           "Sr" , "", SKIN_REFRESH_STATIC },
    
    /* HACK Alert (jdgordon): The next two tags have hacks so we could
     * add a S param at the front without breaking old skins.
     * [SD]D <- handled by the callback, allows SD or S or D params
     * [SI]III[SI]|SN <- SIIIIS|S or IIIIS|S 
     *  keep in sync with parse_touchregion() and parse_lasttouch() */
    { SKIN_TOKEN_LASTTOUCH,             "Tl" , "|[SD]D", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_TOUCHREGION,           "T"  , "[SI]III[SI]|S*", 0|NOBREAK },
    
    { SKIN_TOKEN_HAVE_TOUCH,            "Tp", "", FEATURE_TAG },
    
    { SKIN_TOKEN_CURRENT_SCREEN,        "cs", "", SKIN_REFRESH_DYNAMIC },
    
    { SKIN_TOKEN_HAVE_RECORDING,        "Rp"   , "", FEATURE_TAG },
    { SKIN_TOKEN_IS_RECORDING,          "Rr"   , "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_REC_FREQ,              "Rf"   , "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_REC_ENCODER,           "Re"   , "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_REC_BITRATE,           "Rb"   , "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_REC_MONO,              "Rm"   , "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_REC_SECONDS,           "Rs"   , "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_REC_MINUTES,           "Rn"   , "", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_REC_HOURS,             "Rh"   , "", SKIN_REFRESH_DYNAMIC },
    
    /* Skin variables */
    { SKIN_TOKEN_VAR_SET,               "vs",   "SSi|I", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_VAR_GETVAL,            "vg",   "S", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_VAR_TIMEOUT,           "vl",   "S|D", SKIN_REFRESH_DYNAMIC },

    { SKIN_TOKEN_SUBSTRING,             "ss",   "IiT|s", SKIN_REFRESH_DYNAMIC },
    { SKIN_TOKEN_DRAWRECTANGLE,         "dr",   "IIii|ss", SKIN_REFRESH_STATIC },
    { SKIN_TOKEN_UNKNOWN,                ""   , "", 0 }
    /* Keep this here to mark the end of the table */
};

/* A table of legal escapable characters */
static const char legal_escape_characters[] = "%(,);#<|>";

/*
 * Just does a straight search through the tag table to find one by
 * the given name
 */
const struct tag_info* find_tag(const char* name)
{
    
    const struct tag_info* current = legal_tags;
    
    /* 
     * Continue searching so long as we have a non-empty name string
     * and the name of the current element doesn't match the name
     * we're searching for
     */
    
    while(strcmp(current->name, name) && current->name[0] != '\0')
        current++;

    if(current->name[0] == '\0')
        return NULL;
    else
        return current;

}

/* Searches through the legal escape characters string */
int find_escape_character(char lookup)
{
    const char* current = legal_escape_characters;
    while(*current != lookup && *current != '\0')
        current++;

    if(*current == lookup && *current)
        return 1;
    else
        return 0;
}

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
#define BAR_PARAMS "*|iiiis"
/* The tag definition table */
struct tag_info legal_tags[] = 
{
    { SKIN_TOKEN_ALIGN_CENTER,          "ac", "", 0 },
    { SKIN_TOKEN_ALIGN_LEFT,            "al", "", 0 },
    { SKIN_TOKEN_ALIGN_LEFT_RTL,        "aL", "", 0 },
    { SKIN_TOKEN_ALIGN_RIGHT,           "ar", "", 0 },
    { SKIN_TOKEN_ALIGN_RIGHT_RTL,       "aR", "", 0 },
    { SKIN_TOKEN_ALIGN_LANGDIRECTION,   "ax", "", 0 },
    
    { SKIN_TOKEN_BATTERY_PERCENT,       "bl" , BAR_PARAMS, 0 },
    { SKIN_TOKEN_BATTERY_VOLTS,         "bv", "", 0 },
    { SKIN_TOKEN_BATTERY_TIME,          "bt", "", 0 },
    { SKIN_TOKEN_BATTERY_SLEEPTIME,     "bs", "", 0 },
    { SKIN_TOKEN_BATTERY_CHARGING,      "bc", "", 0 },
    { SKIN_TOKEN_BATTERY_CHARGER_CONNECTED, "bp", "", 0 },
    { SKIN_TOKEN_USB_POWERED,           "bu", "", 0 },
    
    
    { SKIN_TOKEN_RTC_PRESENT,           "cc", "", 0 },
    { SKIN_TOKEN_RTC_DAY_OF_MONTH,      "cd", "", 0 },
    { SKIN_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED, "ce", "", 0 },
    { SKIN_TOKEN_RTC_12HOUR_CFG,        "cf", "", 0 },
    { SKIN_TOKEN_RTC_HOUR_24_ZERO_PADDED, "cH", "", 0 },
    { SKIN_TOKEN_RTC_HOUR_24,           "ck", "", 0 },
    { SKIN_TOKEN_RTC_HOUR_12_ZERO_PADDED, "cI", "", 0 },
    { SKIN_TOKEN_RTC_HOUR_12,           "cl", "", 0 },
    { SKIN_TOKEN_RTC_MONTH,             "cm", "", 0 },
    { SKIN_TOKEN_RTC_MINUTE,            "cM", "", 0 },
    { SKIN_TOKEN_RTC_SECOND,            "cS", "", 0 },
    { SKIN_TOKEN_RTC_YEAR_2_DIGITS,     "cy", "", 0 },
    { SKIN_TOKEN_RTC_YEAR_4_DIGITS,     "cY", "", 0 },
    { SKIN_TOKEN_RTC_AM_PM_UPPER,       "cP", "", 0 },
    { SKIN_TOKEN_RTC_AM_PM_LOWER,       "cp", "", 0 },
    { SKIN_TOKEN_RTC_WEEKDAY_NAME,      "ca", "", 0 },
    { SKIN_TOKEN_RTC_MONTH_NAME,        "cb", "", 0 },
    { SKIN_TOKEN_RTC_DAY_OF_WEEK_START_MON, "cu", "", 0 },
    { SKIN_TOKEN_RTC_DAY_OF_WEEK_START_SUN, "cw", "", 0 },
        
    { SKIN_TOKEN_FILE_BITRATE,          "fb", "", 0 },
    { SKIN_TOKEN_FILE_CODEC,            "fc", "", 0 },
    { SKIN_TOKEN_FILE_FREQUENCY,        "ff", "", 0 },
    { SKIN_TOKEN_FILE_FREQUENCY_KHZ,    "fk", "", 0 },
    { SKIN_TOKEN_FILE_NAME_WITH_EXTENSION,  "fm", "", 0 },
    { SKIN_TOKEN_FILE_NAME,             "fn", "", 0 },
    { SKIN_TOKEN_FILE_PATH,             "fp", "", 0 },
    { SKIN_TOKEN_FILE_SIZE,             "fs", "", 0 },
    { SKIN_TOKEN_FILE_VBR,              "fv", "", 0 },
    { SKIN_TOKEN_FILE_DIRECTORY,        "d"  , "I", 0 },
    
    { SKIN_TOKEN_FILE_BITRATE,          "Fb", "", 0 },
    { SKIN_TOKEN_FILE_CODEC,            "Fc", "", 0 },
    { SKIN_TOKEN_FILE_FREQUENCY,        "Ff", "", 0 },
    { SKIN_TOKEN_FILE_FREQUENCY_KHZ,    "Fk", "", 0 },
    { SKIN_TOKEN_FILE_NAME_WITH_EXTENSION,  "Fm", "", 0 },
    { SKIN_TOKEN_FILE_NAME,             "Fn", "", 0 },
    { SKIN_TOKEN_FILE_PATH,             "Fp", "", 0 },
    { SKIN_TOKEN_FILE_SIZE,             "Fs", "", 0 },
    { SKIN_TOKEN_FILE_VBR,              "Fv", "", 0 },
    { SKIN_TOKEN_FILE_DIRECTORY,        "D"  , "I", 0 },
    
    
    { SKIN_TOKEN_METADATA_ARTIST,       "ia", "", 0 },
    { SKIN_TOKEN_METADATA_COMPOSER,     "ic", "", 0 },
    { SKIN_TOKEN_METADATA_ALBUM,        "id", "", 0 },
    { SKIN_TOKEN_METADATA_ALBUM_ARTIST, "iA", "", 0 },
    { SKIN_TOKEN_METADATA_GROUPING,     "iG", "", 0 },
    { SKIN_TOKEN_METADATA_GENRE,        "ig", "", 0 },
    { SKIN_TOKEN_METADATA_DISC_NUMBER,  "ik", "", 0 },
    { SKIN_TOKEN_METADATA_TRACK_NUMBER, "in", "", 0 },
    { SKIN_TOKEN_METADATA_TRACK_TITLE,  "it", "", 0 },
    { SKIN_TOKEN_METADATA_VERSION,      "iv", "", 0 },
    { SKIN_TOKEN_METADATA_YEAR,         "iy", "", 0 },
    { SKIN_TOKEN_METADATA_COMMENT,      "iC", "", 0 },
    
    { SKIN_TOKEN_METADATA_ARTIST,       "Ia", "", 0 },
    { SKIN_TOKEN_METADATA_COMPOSER,     "Ic", "", 0 },
    { SKIN_TOKEN_METADATA_ALBUM,        "Id", "", 0 },
    { SKIN_TOKEN_METADATA_ALBUM_ARTIST, "IA", "", 0 },
    { SKIN_TOKEN_METADATA_GROUPING,     "IG", "", 0 },
    { SKIN_TOKEN_METADATA_GENRE,        "Ig", "", 0 },
    { SKIN_TOKEN_METADATA_DISC_NUMBER,  "Ik", "", 0 },
    { SKIN_TOKEN_METADATA_TRACK_NUMBER, "In", "", 0 },
    { SKIN_TOKEN_METADATA_TRACK_TITLE,  "It", "", 0 },
    { SKIN_TOKEN_METADATA_VERSION,      "Iv", "", 0 },
    { SKIN_TOKEN_METADATA_YEAR,         "Iy", "", 0 },
    { SKIN_TOKEN_METADATA_COMMENT,      "IC", "", 0 },
    
    { SKIN_TOKEN_SOUND_PITCH,           "Sp", "", 0 },
    { SKIN_TOKEN_SOUND_SPEED,           "Ss", "", 0 },
    
    { SKIN_TOKEN_VLED_HDD,              "lh", "", 0 },
    
    { SKIN_TOKEN_MAIN_HOLD,             "mh", "", 0 },
    { SKIN_TOKEN_REMOTE_HOLD,           "mr", "", 0 },
    { SKIN_TOKEN_REPEAT_MODE,           "mm", "", 0 },
    { SKIN_TOKEN_PLAYBACK_STATUS,       "mp", "", 0 },
    { SKIN_TOKEN_BUTTON_VOLUME,         "mv", "|D", 0 },
    
    { SKIN_TOKEN_PEAKMETER,             "pm", "", 0 },
    { SKIN_TOKEN_PLAYER_PROGRESSBAR,    "pf", "", 0 },
    { SKIN_TOKEN_PROGRESSBAR,           "pb" , BAR_PARAMS, 0 },
    { SKIN_TOKEN_VOLUME,                "pv" , BAR_PARAMS, 0 },
    
    { SKIN_TOKEN_TRACK_ELAPSED_PERCENT, "px", "", 0 },
    { SKIN_TOKEN_TRACK_TIME_ELAPSED,    "pc", "", 0 },
    { SKIN_TOKEN_TRACK_TIME_REMAINING,  "pr", "", 0 },
    { SKIN_TOKEN_TRACK_LENGTH,          "pt", "", 0 },
    { SKIN_TOKEN_TRACK_STARTING,        "pS" , "|D", 0 },
    { SKIN_TOKEN_TRACK_ENDING,          "pE" , "|D", 0 },
    { SKIN_TOKEN_PLAYLIST_POSITION,     "pp", "", 0 },
    { SKIN_TOKEN_PLAYLIST_ENTRIES,      "pe", "", 0 },
    { SKIN_TOKEN_PLAYLIST_NAME,         "pn", "", 0 },
    { SKIN_TOKEN_PLAYLIST_SHUFFLE,      "ps", "", 0 },
    
    { SKIN_TOKEN_DATABASE_PLAYCOUNT,    "rp", "", 0 },
    { SKIN_TOKEN_DATABASE_RATING,       "rr", "", 0 },
    { SKIN_TOKEN_DATABASE_AUTOSCORE,    "ra", "", 0 },
    
    { SKIN_TOKEN_REPLAYGAIN,            "rg", "", 0 },
    { SKIN_TOKEN_CROSSFADE,             "xf", "", 0 },
    
    { SKIN_TOKEN_HAVE_TUNER,            "tp", "", 0 },
    { SKIN_TOKEN_TUNER_TUNED,           "tt", "", 0 },
    { SKIN_TOKEN_TUNER_SCANMODE,        "tm", "", 0 },
    { SKIN_TOKEN_TUNER_STEREO,          "ts", "", 0 },
    { SKIN_TOKEN_TUNER_MINFREQ,         "ta", "", 0 },
    { SKIN_TOKEN_TUNER_MAXFREQ,         "tb", "", 0 },
    { SKIN_TOKEN_TUNER_CURFREQ,         "tf", "", 0 },
    { SKIN_TOKEN_PRESET_ID,             "Ti", "", 0 },
    { SKIN_TOKEN_PRESET_NAME,           "Tn", "", 0 },
    { SKIN_TOKEN_PRESET_FREQ,           "Tf", "", 0 },
    { SKIN_TOKEN_PRESET_COUNT,          "Tc", "", 0 },
    { SKIN_TOKEN_HAVE_RDS,              "tx", "", 0 },
    { SKIN_TOKEN_RDS_NAME,              "ty", "", 0 },
    { SKIN_TOKEN_RDS_TEXT,              "tz", "", 0 },
    
    { SKIN_TOKEN_SUBLINE_SCROLL,        "s", "", 0 },
    { SKIN_TOKEN_SUBLINE_TIMEOUT,       "t"  , "D", 0 },
    
    { SKIN_TOKEN_ENABLE_THEME,          "we", "", NOBREAK },
    { SKIN_TOKEN_DISABLE_THEME,         "wd", "", NOBREAK },
    { SKIN_TOKEN_DRAW_INBUILTBAR,       "wi", "", NOBREAK },
    
    { SKIN_TOKEN_IMAGE_PRELOAD,         "xl", "SFII|I", NOBREAK },
    { SKIN_TOKEN_IMAGE_PRELOAD_DISPLAY, "xd", "S", 0 },
    { SKIN_TOKEN_IMAGE_PRELOAD,         "x", "SFII", NOBREAK },
    
    { SKIN_TOKEN_LOAD_FONT,             "Fl" , "IF", NOBREAK },
    { SKIN_TOKEN_ALBUMART_LOAD,         "Cl" , "IIII|ss", NOBREAK },
    { SKIN_TOKEN_ALBUMART_DISPLAY,      "Cd" , "", 0 },
    { SKIN_TOKEN_ALBUMART_FOUND,        "C" , "", 0 },
    
    { SKIN_TOKEN_VIEWPORT_ENABLE,       "Vd" , "S", 0 },
    { SKIN_TOKEN_UIVIEWPORT_ENABLE,     "VI" , "S", 0 },
    
    { SKIN_TOKEN_VIEWPORT_CUSTOMLIST,   "Vp" , "ICC", NOBREAK },
    { SKIN_TOKEN_LIST_TITLE_TEXT,       "Lt" , "", 0 },
    { SKIN_TOKEN_LIST_TITLE_ICON,       "Li" , "", 0 },
    
    { SKIN_TOKEN_VIEWPORT_FGCOLOUR,       "Vf" , "S", NOBREAK },
    { SKIN_TOKEN_VIEWPORT_BGCOLOUR,       "Vb" , "S", NOBREAK },
    
    { SKIN_TOKEN_VIEWPORT_CONDITIONAL,  "Vl" , "SIIiii", 0 },
    { SKIN_TOKEN_UIVIEWPORT_LOAD,       "Vi" , "sIIiii", 0 },
    { SKIN_TOKEN_VIEWPORT_LOAD,         "V"  , "IIiii", 0 },
    
    { SKIN_TOKEN_IMAGE_BACKDROP,        "X"  , "f", NOBREAK },
    
    { SKIN_TOKEN_SETTING,               "St" , "S", 0 },
    { SKIN_TOKEN_TRANSLATEDSTRING,      "Sx" , "S", 0 },
    { SKIN_TOKEN_LANG_IS_RTL,           "Sr" , "", 0 },
    
    { SKIN_TOKEN_LASTTOUCH,             "Tl" , "|D", 0 },
    { SKIN_TOKEN_CURRENT_SCREEN,        "cs", "", 0 },
    { SKIN_TOKEN_TOUCHREGION,           "T"  , "IIiiS", NOBREAK },
    
    { SKIN_TOKEN_HAVE_RECORDING,        "Rp"   , "", 0 },
    { SKIN_TOKEN_IS_RECORDING,          "Rr"   , "", 0 },
    { SKIN_TOKEN_REC_FREQ,              "Rf"   , "", 0 },
    { SKIN_TOKEN_REC_ENCODER,           "Re"   , "", 0 },
    { SKIN_TOKEN_REC_BITRATE,           "Rb"   , "", 0 },
    { SKIN_TOKEN_REC_MONO,              "Rm"   , "", 0 },
    { SKIN_TOKEN_REC_SECONDS,           "Rs"   , "", 0 },
    { SKIN_TOKEN_REC_MINUTES,           "Rn"   , "", 0 },
    { SKIN_TOKEN_REC_HOURS,             "Rh"   , "", 0 },
    
    { SKIN_TOKEN_UNKNOWN,                ""   , "", 0 }
    /* Keep this here to mark the end of the table */
};

/* A table of legal escapable characters */
char legal_escape_characters[] = "%(,);#<|>";

/*
 * Just does a straight search through the tag table to find one by
 * the given name
 */
struct tag_info* find_tag(char* name)
{
    
    struct tag_info* current = legal_tags;
    
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
    char* current = legal_escape_characters;
    while(*current != lookup && *current != '\0')
        current++;

    if(*current == lookup && *current)
        return 1;
    else
        return 0;
}

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
#define BAR_PARAMS "*|iiiiN"
/* The tag definition table */
struct tag_info legal_tags[] = 
{
    { SKIN_TOKEN_ALIGN_CENTER,          "ac", "" },
    { SKIN_TOKEN_ALIGN_LEFT,            "al", "" },
    { SKIN_TOKEN_ALIGN_LEFT_RTL,        "aL", "" },
    { SKIN_TOKEN_ALIGN_RIGHT,           "ar", "" },
    { SKIN_TOKEN_ALIGN_RIGHT_RTL,       "aR", "" },
    { SKIN_TOKEN_ALIGN_LANGDIRECTION,   "ax", "" },
    
    { SKIN_TOKEN_BATTERY_PERCENT,       "bl" , BAR_PARAMS },
    { SKIN_TOKEN_BATTERY_VOLTS,         "bv", "" },
    { SKIN_TOKEN_BATTERY_TIME,          "bt", "" },
    { SKIN_TOKEN_BATTERY_SLEEPTIME,     "bs", "" },
    { SKIN_TOKEN_BATTERY_CHARGING,      "bc", "" },
    { SKIN_TOKEN_BATTERY_CHARGER_CONNECTED, "bp", "" },
    { SKIN_TOKEN_USB_POWERED,           "bu", "" },
    
    
    { SKIN_TOKEN_RTC_PRESENT,           "cc", "" },
    { SKIN_TOKEN_RTC_DAY_OF_MONTH,      "cd", "" },
    { SKIN_TOKEN_RTC_DAY_OF_MONTH_BLANK_PADDED, "ce", "" },
    { SKIN_TOKEN_RTC_12HOUR_CFG,        "cf", "" },
    { SKIN_TOKEN_RTC_HOUR_24_ZERO_PADDED, "cH", "" },
    { SKIN_TOKEN_RTC_HOUR_24,           "ck", "" },
    { SKIN_TOKEN_RTC_HOUR_12_ZERO_PADDED, "cI", "" },
    { SKIN_TOKEN_RTC_HOUR_12,           "cl", "" },
    { SKIN_TOKEN_RTC_MONTH,             "cm", "" },
    { SKIN_TOKEN_RTC_MINUTE,            "cM", "" },
    { SKIN_TOKEN_RTC_SECOND,            "cS", "" },
    { SKIN_TOKEN_RTC_YEAR_2_DIGITS,     "cy", "" },
    { SKIN_TOKEN_RTC_YEAR_4_DIGITS,     "cY", "" },
    { SKIN_TOKEN_RTC_AM_PM_UPPER,       "cP", "" },
    { SKIN_TOKEN_RTC_AM_PM_LOWER,       "cp", "" },
    { SKIN_TOKEN_RTC_WEEKDAY_NAME,      "ca", "" },
    { SKIN_TOKEN_RTC_MONTH_NAME,        "cb", "" },
    { SKIN_TOKEN_RTC_DAY_OF_WEEK_START_MON, "cu", "" },
    { SKIN_TOKEN_RTC_DAY_OF_WEEK_START_SUN, "cw", "" },
        
    { SKIN_TOKEN_FILE_BITRATE,          "fb", "" },
    { SKIN_TOKEN_FILE_CODEC,            "fc", "" },
    { SKIN_TOKEN_FILE_FREQUENCY,        "ff", "" },
    { SKIN_TOKEN_FILE_FREQUENCY_KHZ,    "fk", "" },
    { SKIN_TOKEN_FILE_NAME_WITH_EXTENSION,  "fm", "" },
    { SKIN_TOKEN_FILE_NAME,             "fn", "" },
    { SKIN_TOKEN_FILE_PATH,             "fp", "" },
    { SKIN_TOKEN_FILE_SIZE,             "fs", "" },
    { SKIN_TOKEN_FILE_VBR,              "fv", "" },
    { SKIN_TOKEN_FILE_DIRECTORY,        "d"  , "I" },
    
    { SKIN_TOKEN_FILE_BITRATE,          "Fb", "" },
    { SKIN_TOKEN_FILE_CODEC,            "Fc", "" },
    { SKIN_TOKEN_FILE_FREQUENCY,        "Ff", "" },
    { SKIN_TOKEN_FILE_FREQUENCY_KHZ,    "Fk", "" },
    { SKIN_TOKEN_FILE_NAME_WITH_EXTENSION,  "Fm", "" },
    { SKIN_TOKEN_FILE_NAME,             "Fn", "" },
    { SKIN_TOKEN_FILE_PATH,             "Fp", "" },
    { SKIN_TOKEN_FILE_SIZE,             "Fs", "" },
    { SKIN_TOKEN_FILE_VBR,              "Fv", "" },
    { SKIN_TOKEN_FILE_DIRECTORY,        "D"  , "I" },
    
    
    { SKIN_TOKEN_METADATA_ARTIST,       "ia", "" },
    { SKIN_TOKEN_METADATA_COMPOSER,     "ic", "" },
    { SKIN_TOKEN_METADATA_ALBUM,        "id", "" },
    { SKIN_TOKEN_METADATA_ALBUM_ARTIST, "iA", "" },
    { SKIN_TOKEN_METADATA_GROUPING,     "iG", "" },
    { SKIN_TOKEN_METADATA_GENRE,        "ig", "" },
    { SKIN_TOKEN_METADATA_DISC_NUMBER,  "ik", "" },
    { SKIN_TOKEN_METADATA_TRACK_NUMBER, "in", "" },
    { SKIN_TOKEN_METADATA_TRACK_TITLE,  "it", "" },
    { SKIN_TOKEN_METADATA_VERSION,      "iv", "" },
    { SKIN_TOKEN_METADATA_YEAR,         "iy", "" },
    { SKIN_TOKEN_METADATA_COMMENT,      "iC", "" },
    
    { SKIN_TOKEN_METADATA_ARTIST,       "Ia", "" },
    { SKIN_TOKEN_METADATA_COMPOSER,     "Ic", "" },
    { SKIN_TOKEN_METADATA_ALBUM,        "Id", "" },
    { SKIN_TOKEN_METADATA_ALBUM_ARTIST, "IA", "" },
    { SKIN_TOKEN_METADATA_GROUPING,     "IG", "" },
    { SKIN_TOKEN_METADATA_GENRE,        "Ig", "" },
    { SKIN_TOKEN_METADATA_DISC_NUMBER,  "Ik", "" },
    { SKIN_TOKEN_METADATA_TRACK_NUMBER, "In", "" },
    { SKIN_TOKEN_METADATA_TRACK_TITLE,  "It", "" },
    { SKIN_TOKEN_METADATA_VERSION,      "Iv", "" },
    { SKIN_TOKEN_METADATA_YEAR,         "Iy", "" },
    { SKIN_TOKEN_METADATA_COMMENT,      "IC", "" },
    
    { SKIN_TOKEN_SOUND_PITCH,           "Sp", "" },
    { SKIN_TOKEN_SOUND_SPEED,           "Ss", "" },
    
    { SKIN_TOKEN_VLED_HDD,              "lh", "" },
    
    { SKIN_TOKEN_MAIN_HOLD,             "mh", "" },
    { SKIN_TOKEN_REMOTE_HOLD,           "mr", "" },
    { SKIN_TOKEN_REPEAT_MODE,           "mm", "" },
    { SKIN_TOKEN_PLAYBACK_STATUS,       "mp", "" },
    { SKIN_TOKEN_BUTTON_VOLUME,         "mv", "|S" },
    
    { SKIN_TOKEN_PEAKMETER,             "pm", "" },
    { SKIN_TOKEN_PLAYER_PROGRESSBAR,    "pf", "" },
    { SKIN_TOKEN_PROGRESSBAR,           "pb" , "*|iiiis" },
    { SKIN_TOKEN_VOLUME,                "pv" , BAR_PARAMS },
    
    { SKIN_TOKEN_TRACK_ELAPSED_PERCENT, "px", "" },
    { SKIN_TOKEN_TRACK_TIME_ELAPSED,    "pc", "" },
    { SKIN_TOKEN_TRACK_TIME_REMAINING,  "pr", "" },
    { SKIN_TOKEN_TRACK_LENGTH,          "pt", "" },
    { SKIN_TOKEN_TRACK_STARTING,        "pS" , "|S"},
    { SKIN_TOKEN_TRACK_ENDING,          "pE" , "|S"},
    { SKIN_TOKEN_PLAYLIST_POSITION,     "pp", "" },
    { SKIN_TOKEN_PLAYLIST_ENTRIES,      "pe", "" },
    { SKIN_TOKEN_PLAYLIST_NAME,         "pn", "" },
    { SKIN_TOKEN_PLAYLIST_SHUFFLE,      "ps", "" },
    
    { SKIN_TOKEN_DATABASE_PLAYCOUNT,    "rp", "" },
    { SKIN_TOKEN_DATABASE_RATING,       "rr", "" },
    { SKIN_TOKEN_DATABASE_AUTOSCORE,    "ra", "" },
    
    { SKIN_TOKEN_REPLAYGAIN,            "rg", "" },
    { SKIN_TOKEN_CROSSFADE,             "xf", "" },
    
    { SKIN_TOKEN_HAVE_TUNER,            "tp", "" },
    { SKIN_TOKEN_TUNER_TUNED,           "tt", "" },
    { SKIN_TOKEN_TUNER_SCANMODE,        "tm", "" },
    { SKIN_TOKEN_TUNER_STEREO,          "ts", "" },
    { SKIN_TOKEN_TUNER_MINFREQ,         "ta", "" },
    { SKIN_TOKEN_TUNER_MAXFREQ,         "tb", "" },
    { SKIN_TOKEN_TUNER_CURFREQ,         "tf", "" },
    { SKIN_TOKEN_PRESET_ID,             "Ti", "" },
    { SKIN_TOKEN_PRESET_NAME,           "Tn", "" },
    { SKIN_TOKEN_PRESET_FREQ,           "Tf", "" },
    { SKIN_TOKEN_PRESET_COUNT,          "Tc", "" },
    { SKIN_TOKEN_HAVE_RDS,              "tx", "" },
    { SKIN_TOKEN_RDS_NAME,              "ty", "" },
    { SKIN_TOKEN_RDS_TEXT,              "tz", "" },
    
    { SKIN_TOKEN_SUBLINE_SCROLL,        "s", "" },
    { SKIN_TOKEN_SUBLINE_TIMEOUT,       "t"  , "S" },
    
    { SKIN_TOKEN_ENABLE_THEME,          "we", "\n" },
    { SKIN_TOKEN_DISABLE_THEME,         "wd", "\n" },
    { SKIN_TOKEN_DRAW_INBUILTBAR,       "wi", "\n" },
    
    { SKIN_TOKEN_IMAGE_PRELOAD,         "xl", "SFII|I\n" },
    { SKIN_TOKEN_IMAGE_PRELOAD_DISPLAY, "xd", "S" },
    { SKIN_TOKEN_IMAGE_PRELOAD,         "x", "SFII\n" },
    
    { SKIN_TOKEN_LOAD_FONT,             "Fl" , "IF\n"},
    { SKIN_TOKEN_ALBUMART_LOAD,         "Cl" , "IIII|ss\n"},
    { SKIN_TOKEN_ALBUMART_DISPLAY,      "Cd" , ""},
    { SKIN_TOKEN_ALBUMART_FOUND,        "C" , ""},
    
    { SKIN_TOKEN_VIEWPORT_ENABLE,       "Vd" , "S"},
    { SKIN_TOKEN_UIVIEWPORT_ENABLE,     "VI" , "S"},
    
    { SKIN_TOKEN_VIEWPORT_CUSTOMLIST,   "Vp" , "ICC\n"},
    { SKIN_TOKEN_LIST_TITLE_TEXT,       "Lt" , ""},
    { SKIN_TOKEN_LIST_TITLE_ICON,       "Li" , ""},
    
    { SKIN_TOKEN_VIEWPORT_FGCOLOUR,       "Vf" , "S"},
    { SKIN_TOKEN_VIEWPORT_BGCOLOUR,       "Vb" , "S"},
    
    { SKIN_TOKEN_VIEWPORT_CONDITIONAL,  "Vl" , "SIIiii"},
    { SKIN_TOKEN_UIVIEWPORT_LOAD,       "Vi" , "sIIiii"},
    { SKIN_TOKEN_VIEWPORT_LOAD,         "V"  , "IIiii"},
    
    { SKIN_TOKEN_IMAGE_BACKDROP,        "X"  , "f\n"},
    
    { SKIN_TOKEN_SETTING,               "St" , "S"},
    { SKIN_TOKEN_TRANSLATEDSTRING,      "Sx" , "S"},
    { SKIN_TOKEN_LANG_IS_RTL,           "Sr" , ""},
    
    { SKIN_TOKEN_LASTTOUCH,             "Tl" , "|S"},
    { SKIN_TOKEN_CURRENT_SCREEN,        "cs", "" },
    { SKIN_TOKEN_TOUCHREGION,           "T"  , "IIiiS\n"},
    
    { SKIN_TOKEN_HAVE_RECORDING,        "Rp"   , ""},
    { SKIN_TOKEN_IS_RECORDING,          "Rr"   , ""},
    { SKIN_TOKEN_REC_FREQ,              "Rf"   , ""},
    { SKIN_TOKEN_REC_ENCODER,           "Re"   , ""},
    { SKIN_TOKEN_REC_BITRATE,           "Rb"   , ""},
    { SKIN_TOKEN_REC_MONO,              "Rm"   , ""},
    { SKIN_TOKEN_REC_SECONDS,           "Rs"   , ""},
    { SKIN_TOKEN_REC_MINUTES,           "Rn"   , ""},
    { SKIN_TOKEN_REC_HOURS,             "Rh"   , ""},
    
    { SKIN_TOKEN_UNKNOWN,                ""   , ""}
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

/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jonathan Gordon
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
#ifndef __SETTINGSLIST_H
#define __SETTINGSLIST_H
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>
#include "inttypes.h"

typedef int (*_isfunc_type)(void);

union storage_type {
    int int_;
    unsigned int uint_;
    bool bool_;
    char *charptr;
    unsigned char *ucharptr;
    _isfunc_type func;
    void* custom;
};
/* the variable type for the setting */
#define F_T_INT      1
#define F_T_UINT     2
#define F_T_BOOL     3
#define F_T_CHARPTR  4
#define F_T_UCHARPTR 5
#define F_T_CUSTOM   6 /* MUST use struct custom_setting below */
#define F_T_MASK     0x7

struct sound_setting {
    int setting; /* from the enum in firmware/sound.h */
};
#define F_T_SOUND    0x8 /* this variable uses the set_sound stuff,         \
                            | with one of the above types (usually F_T_INT) \
                            These settings get the default from sound_default(setting); */
struct bool_setting {
    void (*option_callback)(bool);
    int lang_yes;
    int lang_no;
};
#define F_BOOL_SETTING (F_T_BOOL|0x10)
#define F_RGB 0x20

struct filename_setting {
    const char* prefix;
    const char* suffix;
    int max_len;
};
#define F_FILENAME 0x40

struct int_setting {
    void (*option_callback)(int);
    int unit;
    int min;
    int max;
    int step;
    const char* (*formatter)(char*, size_t, int, const char*);
    int32_t (*get_talk_id)(int, int);
};
#define F_INT_SETTING 0x80

struct choice_setting {
    void (*option_callback)(int);
    int count;
    union {
        const unsigned char **desc;
        const int           *talks;
    };
};
#define F_CHOICE_SETTING 0x100
#define F_CHOICETALKS    0x200 /* uses .talks in the above struct for the talks */
                               /* and cfg_vals for the strings to display */

struct table_setting {
    void (*option_callback)(int);
    const char* (*formatter)(char*, size_t, int, const char*);
    int32_t (*get_talk_id)(int, int);
    int unit;
    int count;
    const int * values;
};
#define F_TABLE_SETTING 0x2000
#define F_ALLOW_ARBITRARY_VALS 0x4000
/* these use the _isfunc_type type for the function */
/* typedef int (*_isfunc_type)(void); */
#define F_MIN_ISFUNC    0x100000 /* min(above) is function pointer to above type */
#define F_MAX_ISFUNC    0x200000 /* max(above) is function pointer to above type */
#define F_DEF_ISFUNC    0x400000 /* default_val is function pointer to above type */

/* The next stuff is used when none of the other types work.
   Should really only be used if the value you want to store in global_settings
   is very different to the string you want to use in the config. */
#define F_CUSTOM_SETTING 0x8000
struct custom_setting {
    /* load the saved value from the .cfg
        setting: pointer into global_settings
        value: the text from the .cfg
     */
    void (*load_from_cfg)(void* setting, char*value);
    /* store the value into a .cfg
        setting: pointer into global_settings
        buf/buf_len: buffer and length to write the string into.
       Returns the string.
     */
    char* (*write_to_cfg)(void* setting, char*buf, int buf_len);
    /* Check if the setting has been changed from the default.
        setting: pointer into global_settings
        defaultval: the value given in the settings_list.c macro
       Return true if the setting was changed
     */
    bool (*is_changed)(void* setting, void* defaultval);
    /* Set the setting back to its default value.
        setting: pointer into global_settings
        defaultval: the value given in the settings_list.c macro
     */
    void (*set_default)(void* setting, void* defaultval);
};

#define F_THEMESETTING  0x0800000
#define F_RECSETTING    0x1000000
#define F_EQSETTING     0x2000000
#define F_SOUNDSETTING  0x4000000

#define F_NVRAM_BYTES_MASK     0xE0000 /*0-4 bytes can be stored */
#define F_NVRAM_MASK_SHIFT     17
#define NVRAM_CONFIG_VERSION   9
/* Above define should be bumped if
- a new NVRAM setting is added between 2 other NVRAM settings
- number of bytes for a NVRAM setting is changed
- a NVRAM setting is removed
*/
#define F_TEMPVAR    0x0400 /* used if the setting should be set using a temp var */
#define F_PADTITLE   0x800 /* pad the title with spaces to force it to scroll */
#define F_NO_WRAP     0x1000 /* used if the list should not wrap */

#define F_BANFROMQS     0x80000000 /* ban the setting from the quickscreen items */
#define F_DEPRECATED    0x40000000 /* DEPRECATED setting, don't write to .cfg */
struct settings_list {
    uint32_t             flags;   /* BD__ _SER TFFF NNN_ _ATW PTVC IFRB STTT */
    void                *setting;
    int                  lang_id; /* -1 for none */
    union storage_type   default_val;
    const char          *cfg_name; /* this settings name in the cfg file   */
    const char          *cfg_vals; /*comma seperated legal values, or NULL */
                                   /* used with F_T_UCHARPTR this is the folder prefix */
    union {
        const void *RESERVED; /* to stop compile errors, will be removed */
        const struct sound_setting *sound_setting; /* use F_T_SOUND for this */
        const struct bool_setting  *bool_setting; /* F_BOOL_SETTING */
        const struct filename_setting *filename_setting; /* use F_FILENAME */
        const struct int_setting *int_setting; /* use F_INT_SETTING */
        const struct choice_setting *choice_setting; /* F_CHOICE_SETTING */
        const struct table_setting *table_setting; /* F_TABLE_SETTING */
        const struct custom_setting *custom_setting; /* F_CUSTOM_SETTING */
    };
};
const struct settings_list* get_settings_list(int*count);
#ifndef PLUGIN
/* not needed for plugins and just causes compile error,
   possibly fix proberly later */
extern const struct settings_list  settings[];
extern const int nb_settings;

#endif

#endif

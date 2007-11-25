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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
};
/* the variable type for the setting */
#define F_T_INT      1
#define F_T_UINT     2
#define F_T_BOOL     3
#define F_T_CHARPTR  4
#define F_T_UCHARPTR 5
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
    void (*formatter)(char*, size_t, int, const char*);
    int32_t (*get_talk_id)(int);
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
    void (*formatter)(char*, size_t, int, const char*);
    int32_t (*get_talk_id)(int);
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

#define F_THEMESETTING  0x0800000
#define F_RECSETTING    0x1000000

#define F_NVRAM_BYTES_MASK     0xE0000 /*0-4 bytes can be stored */
#define F_NVRAM_MASK_SHIFT     17
#define NVRAM_CONFIG_VERSION 4
/* Above define should be bumped if
- a new NVRAM setting is added between 2 other NVRAM settings
- number of bytes for a NVRAM setting is changed
- a NVRAM setting is removed
*/
#define F_TEMPVAR    0x0400 /* used if the setting should be set using a temp var */
#define F_PADTITLE   0x800 /* pad the title with spaces to force it to scroll */
#define F_NO_WRAP     0x1000 /* used if the list should not wrap */

struct settings_list {
    uint32_t             flags;   /* ____ ___R TFFF NNN_ _ATW PTVC IFRB STTT */
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
    };
};

#ifndef PLUGIN
/* not needed for plugins and just causes compile error,
   possibly fix proberly later */
extern const struct settings_list  settings[];
extern const int nb_settings;

#endif

#endif

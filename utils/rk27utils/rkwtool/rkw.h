/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Marcin Bukat
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

#include <stdint.h>

#define RKLD_MAGIC 0x4C44524B
#define RKRS_MAGIC 0x53524B52
#define RKST_MAGIC 0X53544B52

enum section_type_t {
    ST_RKLD,
    ST_RKRS,
    ST_RKST
};

enum rkst_action_t {
    act_null = 0,
    act_mkdir = 1,
    act_fcopy = 2,
    act_fsoper = 3,
    act_format = 4,
    act_loader = 5,

    act_dispbmp = 10,
    act_dispstr = 11,
    act_setfont = 12,

    act_delay = 20,

    act_system = 100,
    act_uilogo = 100,
    act_readme = 101,
    act_copyright = 102,
    act_select = 103,
    act_restart = 104,

    act_regkey = 120,
    act_version = 121,

    act_freplace = 130,
    act_fpreplace = 131,
    act_fsdel = 132,

    act_space = 200,

    act_addfile = 300,

    act_setmem = 1000,
    act_getmem = 1001,
};

struct section_header_t {
    uint32_t size;
    uint32_t magic;
    uint32_t property;
    uint32_t timestamp;
    uint32_t allign;
    uint32_t file_size;
    uint16_t size_of_name_dir;
    uint16_t size_of_id_dir;
    uint16_t number_of_named_entries;
    uint16_t number_of_id_entries;
    uint32_t offset_of_named_entries;
    uint32_t offset_of_id_entries;
} __attribute__((__packed__));

struct rkrs_named_t {
    uint32_t size;
    uint32_t type;
    uint32_t data_offset;
    uint32_t data_size;
    uint32_t param[4];
} __attribute__((__packed__));

struct rkst_named_t {
    uint32_t size;
    uint32_t action;
    uint32_t data_offset;
    uint32_t data_size;
    uint8_t name;
};

struct section_info_t {
    char *header;
    char *items;
};

struct rkw_info_t {
    char *rkw;
    long size;
    struct section_info_t rkrs_info;
    struct section_info_t rkst_info;
};

/* general functions */
struct rkw_info_t *rkw_slurp(char *filename);
void rkw_free(struct rkw_info_t *rkw_info);

/* info functions */
void rkrs_list_named_items(struct rkw_info_t *rkw_info);
void rkst_list_named_items(struct rkw_info_t *rkw_info);

/* extract functions */
void unpack_bootloader(struct rkw_info_t *rkw_info, char *prefix);
void unpack_rkst(struct rkw_info_t *rkw_info, char *prefix);
void unpack_addfile(struct rkw_info_t *rkw_info, char *prefix);

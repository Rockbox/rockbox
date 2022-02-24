/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2020 by William Wilgus
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
#ifndef CORE_KEYMAP_H
#define CORE_KEYMAP_H

#include <stdbool.h>
#include <inttypes.h>
#include "config.h"
#include "action.h"
#define KEYREMAP_VERSION 1
#define KEYREMAP_HEADERID (LAST_ACTION_PLACEHOLDER | (TARGET_ID << 8))

/* If exists remap file will be loaded at startup */
#define CORE_KEYREMAP_FILE ROCKBOX_DIR "/keyremap.kmf"

/* Allocates core buffer, copies keymap to allow buttons for actions to be remapped*/
int core_set_keyremap(struct button_mapping* core_keymap, int count);

/* open_key_remap(filename , *fd (you must close file_descriptor), *fsize)
 * checks/strips header and returns remaining count
 * fd is opened and set to first record
 * filesize contains the size of the remaining records
*/
int open_key_remap(const char *filename, int *fd, size_t *filesize);

/* load a remap file to allow buttons for actions to be remapped */
int core_load_key_remap(const char *filename);

/*
 * entries consist of 3 int [action, button, prebtn]
 * the header (VERSION, LAST_DEFINED_ACTION, count) is stripped by open_key_remap
 *
 * context look up table is at the beginning
 * action_code contains (context | CONTEXT_REMAPPED)
 * button_code contains index of first remapped action for the matched context
 * prebtn_code contains count of actions in this remapped context
 * [-1] REMAP_VERSION, REMAP_HEADERID, entry count(9) / DISCARDED AFTER LOAD
 * [0] CORE_CONTEXT_REMAP(ctx1), offset1=(3), count=(1)
 * [1] CORE_CONTEXT_REMAP(ctx2, offset2=(5), count=(2)
 * [2] sentinel, 0, 0
 * [3] act0, btn, 0
 * [4] sentinel 0, 0
 * [5] act1, btn, 0
 * [6] act2, btn1
 * [7] sentinel, 0, 0
 *
 * Note:
 * last entry of each group is always the sentinel [CONTEXT_STOPSEARCHING, BUTTON_NONE, BUTTON_NONE]
 * contexts must match exactly -- re-mapped contexts run before the built in w/ fall through contexts
 * ie. you can't remap std_context and expect it to match std_context actions from the WPS context.
 */

#endif /* CORE_KEYMAP_H */


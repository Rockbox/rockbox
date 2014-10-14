/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei, Benjamin Brown
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "intern.h"
#include "vm.h"
#include "mixer.h"
#include "sfxplayer.h"
#include "resource.h"
#include "video.h"
#include "sys.h"

#define STRING_TABLE_FILE "xworld.strings" /* this is relative to dataDir */
#define STRING_TABLE_VERSION 0x03

#define XWORLD_FONT_FILE "xworld.font" /* relative to dataDir */
#define XWORLD_FONT_VERSION 0x01

struct System;

#define MAX_SAVE_SLOTS 1
#define SAVE_MAGIC 0x42424657
struct Engine {
    struct System *sys;
    struct VirtualMachine vm;
    struct Mixer mixer;
    struct Resource res;
    struct SfxPlayer player;
    struct Video video;
    const char *_dataDir, *_saveDir;
    uint8_t _stateSlot;
};

void engine_create(struct Engine* e, struct System* stub, const char* dataDir, const char* saveDir);

void engine_run(struct Engine*);
void engine_init(struct Engine*);
void engine_finish(struct Engine*);
void engine_processInput(struct Engine*);

bool engine_loadFontFile(struct Engine*);
bool engine_loadStringTable(struct Engine*);

void engine_makeGameStateName(struct Engine*, uint8_t slot, char *buf, int sz);
void engine_saveGameState(struct Engine*, uint8_t slot, const char *desc);
bool engine_loadGameState(struct Engine*, uint8_t slot);
void engine_deleteGameState(struct Engine*, uint8_t slot);
const char* engine_getDataDir(struct Engine*);
#endif

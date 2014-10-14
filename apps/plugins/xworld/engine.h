/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "intern.h"
#include "vm.h"
#include "mixer.h"
#include "sfxplayer.h"
#include "resource.h"
#include "video.h"
#include "sys.h"

struct System;

#define MAX_SAVE_SLOTS 100
/* "AWSV": */
#define SAVE_MAGIC 0x414D5356
struct Engine {
    struct System *sys;
    struct VirtualMachine *vm;
    struct Mixer mixer;
    struct Resource res;
    struct SfxPlayer player;
    struct Video video;
    const char *_dataDir, *_saveDir;
    uint8_t _stateSlot;

};

//Engine(System *stub, const char *dataDir, const char *saveDir);
/* ASSUMES MEMORY ALREADY ALLOCATED!!! */
void engine_create(struct Engine* e, struct System* stub, const char* dataDir, const char* saveDir);

void engine_run(struct Engine*);
void engine_init(struct Engine*);
void engine_finish(struct Engine*);
void engine_processInput(struct Engine*);

void engine_makeGameStateName(struct Engine*, uint8_t slot, char *buf, int sz);
void engine_saveGameState(struct Engine*, uint8_t slot, const char *desc);
void engine_loadGameState(struct Engine*, uint8_t slot);
const char* engine_getDataDir(struct Engine*);
#endif

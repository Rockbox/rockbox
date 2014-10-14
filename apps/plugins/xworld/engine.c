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

#include "engine.h"
#include "file.h"
#include "serializer.h"
#include "sys.h"
#include "parts.h"

/*
engine_Engine(System *paramSys, const char *dataDir, const char *saveDir)
: sys(paramSys), vm(&mixer, &res, &player, &video, sys), mixer(sys), res(&video, dataDir),
    player(&mixer, &res, sys), video(&res, sys), _dataDir(dataDir), _saveDir(saveDir), _stateSlot(0) {
}
*/
void engine_create(struct Engine* e, struct System* stub, const char* dataDir, const char* saveDir)
{
    debug(DBG_SYS, "Start engine_create");
    e->sys=stub;
    debug(DBG_SYS, "engine_create Point 0");
    e->sys->e=e;
    debug(DBG_SYS, "engine_create Point 1");
    e->_dataDir=dataDir;
    debug(DBG_SYS, "engine_create Point 2");
    e->_saveDir=saveDir;
    debug(DBG_SYS, "engine_create Point 3");
    mixer_create(&e->mixer, e->sys);
    debug(DBG_SYS, "engine_create Point 4");
    /* this needs to come as early as possible to allocate space for the vm struct: */
    res_create(&e->res, &e->video, dataDir);
    res_allocMemBlock(&e->res);

    e->vm = e->res._memPtrStart + (4 * VID_PAGE_SIZE);

    debug(DBG_SYS, "engine_create Point 5");
    video_create(&e->video, &e->res, e->sys);
    debug(DBG_SYS, "engine_create Point 6");
    player_create(&e->player, &e->mixer, &e->res, e->sys);
    debug(DBG_SYS, "engine_create Point 7");
    vm_create(e->vm, &e->mixer, &e->res, &e->player, &e->video, e->sys);
    debug(DBG_SYS, "End engine_create");
}

void engine_run(struct Engine* e) {

    while (!e->sys->input.quit) {

        vm_checkThreadRequests(e->vm);

        vm_inp_updatePlayer(e->vm);

        engine_processInput(e);

        vm_hostFrame(e->vm);
    }


}

void engine_init(struct Engine* e) {
    debug(DBG_SYS, "Start engine_init");
    //Init system
    sys_init(e->sys, "Out Of This World");
    debug(DBG_SYS, "engine_init Point 0");

    video_init(&e->video);
    debug(DBG_SYS, "engine_init Point 2");

    res_readEntries(&e->res);
    debug(DBG_SYS, "engine_init Point 3");

    vm_init(e->vm);
    debug(DBG_SYS, "engine_init Point 4");

    mixer_init(&e->mixer);
    debug(DBG_SYS, "engine_init Point 5");

    player_init(&e->player);
    debug(DBG_SYS, "engine_init Point 6");

    //Init virtual machine, legacy way
    vm_initForPart(e->vm, GAME_PART_FIRST); // This game part is the protection screen

    // Try to cheat here. You can jump anywhere but the VM crashes afterward.
    // Starting somewhere is probably not enough, the variables and calls return are probably missing.
    //vm_initForPart(e->vm, GAME_PART2); // Skip protection screen and go directly to intro
    //vm_initForPart(e->vm, GAME_PART3); // CRASH
    //vm_initForPart(e->vm, GAME_PART4); // Start directly in jail but then crash
    //vm->initForPart(e->vm, GAME_PART5);   //CRASH
    //vm->initForPart(GAME_PART6);   // Start in the battlechar but CRASH afteward
    //vm->initForPart(GAME_PART7); //CRASH
    //vm->initForPart(GAME_PART8); //CRASH
    //vm->initForPart(GAME_PART9); // Green screen not doing anything
    rb->splashf(HZ, "End engine_init");

}

void engine_finish(struct Engine* e) {
    player_free(&e->player);
    mixer_free(&e->mixer);
    res_freeMemBlock(&e->res);
}

void engine_processInput(struct Engine* e) {
    if (e->sys->input.load) {
        engine_loadGameState(e, e->_stateSlot);
        e->sys->input.load = false;
    }
    if (e->sys->input.save) {
        engine_saveGameState(e, e->_stateSlot, "quicksave");
        e->sys->input.save = false;
    }
    if (e->sys->input.fastMode) {
        e->vm->_fastMode = !e->vm->_fastMode;
        e->sys->input.fastMode = false;
    }
    if (e->sys->input.stateSlot != 0) {
        int8_t slot = e->_stateSlot + e->sys->input.stateSlot;
        if (slot >= 0 && slot < MAX_SAVE_SLOTS) {
            e->_stateSlot = slot;
            debug(DBG_INFO, "Current game state slot is %d", e->_stateSlot);
        }
        e->sys->input.stateSlot = 0;
    }
}

void engine_makeGameStateName(struct Engine* e, uint8_t slot, char *buf, int sz) {
    (void) e;
    rb->snprintf(buf, sz, "raw.s%02d", slot);
}

void engine_saveGameState(struct Engine* e, uint8_t slot, const char *desc) {
    char stateFile[20];
    engine_makeGameStateName(e, slot, stateFile, 20);
    File f;
    file_create(&f, true);
    if (!file_open(&f, stateFile, e->_saveDir, "wb")) {
        warning("Unable to save state file '%s'", stateFile);
    } else {
        // header
        file_writeUint32BE(&f, SAVE_MAGIC);
        file_writeUint16BE(&f, CUR_VER);
        file_writeUint16BE(&f, 0);
        char hdrdesc[32];
        strncpy(hdrdesc, desc, sizeof(hdrdesc) - 1);
        file_write(&f, hdrdesc, sizeof(hdrdesc));
        // contents
        struct Serializer s;
        ser_create(&s, &f, SM_SAVE, e->res._memPtrStart, CUR_VER);
        vm_saveOrLoad(e->vm, &s);
        res_saveOrLoad(&e->res, &s);
        video_saveOrLoad(&e->video, &s);
        player_saveOrLoad(&e->player, &s);
        mixer_saveOrLoad(&e->mixer, &s);
        if (file_ioErr(&f)) {
            warning("I/O error when saving game state");
        } else {
            debug(DBG_INFO, "Saved state to slot %d", e->_stateSlot);
        }
    }
    file_close(&f);
}

void engine_loadGameState(struct Engine* e, uint8_t slot) {
    char stateFile[20];
    engine_makeGameStateName(e, slot, stateFile, 20);
    File f;
    file_create(&f, true);
    if (!file_open(&f, stateFile, e->_saveDir, "rb")) {
        warning("Unable to open state file '%s'", stateFile);
    } else {
        uint32_t id = file_readUint32BE(&f);
        if (id != SAVE_MAGIC) {
            warning("Bad savegame format");
        } else {
            // mute
            player_stop(&e->player);
            mixer_stopAll(&e->mixer);
            // header
            uint16_t ver = file_readUint16BE(&f);
            file_readUint16BE(&f);
            char hdrdesc[32];
            file_read(&f, hdrdesc, sizeof(hdrdesc));
            // contents
            struct Serializer s;
            ser_create(&s, &f, SM_LOAD, e->res._memPtrStart, ver);
            vm_saveOrLoad(e->vm, &s);
            res_saveOrLoad(&e->res, &s);
            video_saveOrLoad(&e->video, &s);
            player_saveOrLoad(&e->player, &s);
            mixer_saveOrLoad(&e->mixer, &s);
        }
        if (file_ioErr(&f)) {
            warning("I/O error when loading game state");
        } else {
            debug(DBG_INFO, "Loaded state from slot %d", e->_stateSlot);
        }
    }
    file_close(&f);
}


const char* engine_getDataDir(struct Engine* e)
{
    return e->_dataDir;
}

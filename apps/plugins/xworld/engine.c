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
    vm_create(&e->vm, &e->res, &e->video, e->sys);
    res_create(&e->res, &e->video, e->dataDir);
    e->sys=stub;
    e->_dataDir=dataDir;
    e->_saveDir=saveDir;
}

void engine_run(struct Engine* e) {

    while (!e->sys->input.quit) {

        vm_checkThreadRequests(&e->vm);

        vm_inp_updatePlayer(&e->vm);

        engine_processInput(e);

        vm_hostFrame(&e->vm);
    }


}
void engine_create(struct System* stub, struct Engine*, const char* dataDir, const char* saveDir)
{
    e->sys=stub;
    e->_dataDir=dataDir;
    e->_saveDir=saveDir;
}
void engine_init() {


    //Init system
    sys_init("Out Of This World");

    video_init(&e->video);

    res_allocMemBlock();

    res_readEntries();

    vm_create(&e->vm, &e->res, &e->video, e->sys);

    vm_init(&e->vm);

    //mixer.init();

    //player.init();

    //Init virtual machine, legacy way
    vm_initForPart(&e->vm, GAME_PART_FIRST); // This game part is the protection screen



    // Try to cheat here. You can jump anywhere but the VM crashes afterward.
    // Starting somewhere is probably not enough, the variables and calls return are probably missing.
    //vm.initForPart(GAME_PART2); // Skip protection screen and go directly to intro
    //vm.initForPart(GAME_PART3); // CRASH
    //vm.initForPart(GAME_PART4); // Start directly in jail but then crash
    //vm.initForPart(GAME_PART5);   //CRASH
    //vm.initForPart(GAME_PART6);   // Start in the battlechar but CRASH afteward
    //vm.initForPart(GAME_PART7); //CRASH
    //vm.initForPart(GAME_PART8); //CRASH
    //vm.initForPart(GAME_PART9); // Green screen not doing anything
}

void engine_finish(struct Engine* e) {
    //player.free();
    //mixer.free();
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
        e->vm._fastMode = !e->vm._fastMode;
        e->sys->input.fastMode = false;
    }
    if (e->sys->input.stateSlot != 0) {
        int8_t slot = e->_stateSlot + e->sys->input.stateSlot;
        if (slot >= 0 && slot < MAX_SAVE_SLOTS) {
            e->_stateSlot = slot;
            debug(DBG_INFO, "Current game state slot is %d", _stateSlot);
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
    File f(true);
    if (!f.open(stateFile, _saveDir, "wb")) {
        warning("Unable to save state file '%s'", stateFile);
    } else {
        // header
        f.writeUint32BE('AWSV');
        f.writeUint16BE(Serializer::CUR_VER);
        f.writeUint16BE(0);
        char hdrdesc[32];
        strncpy(hdrdesc, desc, sizeof(hdrdesc) - 1);
        f.write(hdrdesc, sizeof(hdrdesc));
        // contents
        Serializer s(&f, Serializer::SM_SAVE, res._memPtrStart);
        vm.saveOrLoad(s);
        res.saveOrLoad(s);
        video.saveOrLoad(s);
        player.saveOrLoad(s);
        mixer.saveOrLoad(s);
        if (f.ioErr()) {
            warning("I/O error when saving game state");
        } else {
            debug(DBG_INFO, "Saved state to slot %d", _stateSlot);
        }
    }
}

void engine_loadGameState(struct Engine* e, uint8_t slot) {
    char stateFile[20];
    engine_makeGameStateName(e, slot, stateFile);
    File f(true);
    if (!f.open(stateFile, e->_saveDir, "rb")) {
        warning("Unable to open state file '%s'", stateFile);
    } else {
        uint32_t id = f.readUint32BE();
        if (id != 'AWSV') {
            warning("Bad savegame format");
        } else {
            // mute
            player.stop();
            mixer.stopAll();
            // header
            uint16_t ver = f.readUint16BE();
            f.readUint16BE();
            char hdrdesc[32];
            f.read(hdrdesc, sizeof(hdrdesc));
            // contents
            Serializer s(&f, Serializer::SM_LOAD, res._memPtrStart, ver);
            vm.saveOrLoad(s);
            res.saveOrLoad(s);
            video.saveOrLoad(s);
            player.saveOrLoad(s);
            mixer.saveOrLoad(s);
        }
        if (f.ioErr()) {
            warning("I/O error when loading game state");
        } else {
            debug(DBG_INFO, "Loaded state from slot %d", _stateSlot);
        }
    }
}


const char* engine_getDataDir(struct Engine* e)
{
    return e->_dataDir;
}

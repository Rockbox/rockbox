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

#include "plugin.h"
#include "engine.h"
#include "file.h"
#include "serializer.h"
#include "sys.h"
#include "parts.h"
#include "video_data.h"
#include "video.h"

void engine_create(struct Engine* e, struct System* stub, const char* dataDir, const char* saveDir)
{
    e->sys = stub;
    e->sys->e = e;
    e->_dataDir = dataDir;
    e->_saveDir = saveDir;

    mixer_create(&e->mixer, e->sys);

    /* this needs to be here and not engine_init() to ensure that it is not called on a reset */
    res_create(&e->res, &e->video, e->sys, dataDir);

    res_allocMemBlock(&e->res);

    video_create(&e->video, &e->res, e->sys);

    player_create(&e->player, &e->mixer, &e->res, e->sys);

    vm_create(&e->vm, &e->mixer, &e->res, &e->player, &e->video, e->sys);
}

void engine_run(struct Engine* e) {

    while (!e->sys->input.quit) {

        vm_checkThreadRequests(&e->vm);

        vm_inp_updatePlayer(&e->vm);

        engine_processInput(e);

        vm_hostFrame(&e->vm);

        /* only yield() in the whole game :P */
        rb->yield();
    }

}

/*
 * this function loads the font in XWORLD_FONT_FILE into video_font
 */

/*
 * the file format for the font file is like this:
 * "XFNT" magic
 * 8-bit version number
 * <768 bytes data>
 * sum of data, XOR'ed by version number repeated 4 times (32-bit)
 */
bool engine_loadFontFile(struct Engine* e)
{
    uint8_t *old_font = sys_get_buffer(e->sys, sizeof(video_font));
    rb->memcpy(old_font, video_font, sizeof(video_font));

    File f;
    file_create(&f, false);
    if(!file_open(&f, XWORLD_FONT_FILE, e->_dataDir, "rb"))
    {
        goto fail;
    }

    /* read header */
    char header[5];
    int ret = file_read(&f, header, sizeof(header));
    if(ret != sizeof(header) ||
            header[0] != 'X' ||
            header[1] != 'F' ||
            header[2] != 'N' ||
            header[3] != 'T')
    {
        warning("Invalid font file signature, falling back to alternate font");
        goto fail;
    }

    if(header[4] != XWORLD_FONT_VERSION)
    {
        warning("Font file version mismatch (have=%d, need=%d), falling back to alternate font", header[4], XWORLD_FONT_VERSION);
        goto fail;
    }

    uint32_t sum = 0;
    for(unsigned int i = 0;i<sizeof(video_font);++i)
    {
        sum += video_font[i] = file_readByte(&f);
    }

    uint32_t mask = (header[4] << 24) |
                    (header[4] << 16) |
                    (header[4] << 8 ) |
                    (header[4] << 0 );
    sum ^= mask;
    uint32_t check = file_readUint32BE(&f);

    if(check != sum)
    {
        warning("Bad font checksum, falling back to alternate font");
        goto fail;
    }

    file_close(&f);
    return true;

fail:
    file_close(&f);

    memcpy(video_font, old_font, sizeof(video_font));
    return false;
}

/*
 * this function loads the string table in STRING_TABLE_FILE into
 * video_stringsTableEng
 */

/*
 * the file format for the string table is like this:
 * "XWST" magic
 * 8-bit version number
 * 8-bit title length
 * <title data (0-255 bytes, _NO NULL_)
 * 16-bit number of string entries (currently limited to 255)
 * entry format:
   struct file_entry_t
   {
     uint16_t id;
     uint16_t len; - length of str
     char* str;    - NO NULL
   }
*/
bool engine_loadStringTable(struct Engine* e)
{
    File f;
    file_create(&f, false);
    if(!file_open(&f, STRING_TABLE_FILE, e->_dataDir, "rb"))
    {
        /*
         * this gives verbose warnings while loadFontFile doesn't because the font looks similar
         * enough to pass for the "original", but the strings don't
         */
        /* FW 2017-2-12: eliminated obnoxious warning */
        /*warning("Unable to find string table, falling back to alternate strings");*/
        goto fail;
    }

    /* read header */

    char header[5];
    int ret = file_read(&f, header, sizeof(header));
    if(ret != sizeof(header) ||
            header[0] != 'X' ||
            header[1] != 'W' ||
            header[2] != 'S' ||
            header[3] != 'T')
    {
        warning("Invalid string table signature, falling back to alternate strings");
        goto fail;
    }

    if(header[4] != STRING_TABLE_VERSION)
    {
        warning("String table version mismatch (have=%d, need=%d), falling back to alternate strings", header[4], STRING_TABLE_VERSION);
        goto fail;
    }

    /* read title */

    uint8_t title_length = file_readByte(&f);
    char *title_buf;
    if(title_length)
    {
        title_buf = sys_get_buffer(e->sys, (int32_t)title_length + 1); /* make room for the NULL */
        ret = file_read(&f, title_buf, title_length);
        if(ret != title_length)
        {
            warning("Title shorter than expected, falling back to alternate strings");
            goto fail;
        }
    }
    else
    {
        title_buf = "UNKNOWN";
    }

    /* read entries */

    uint16_t num_entries = file_readUint16BE(&f);
    for(unsigned int i = 0; i < num_entries && i < ARRAYLEN(video_stringsTableEng); ++i)
    {
        video_stringsTableEng[i].id = file_readUint16BE(&f);
        uint16_t len                = file_readUint16BE(&f);

        if(file_ioErr(&f))
        {
            warning("Unexpected EOF in while parsing entry %d, falling back to alternate strings", i);
            goto fail;
        }

        video_stringsTableEng[i].str = sys_get_buffer(e->sys, (int32_t)len + 1);

        ret = file_read(&f, video_stringsTableEng[i].str, len);
        if(ret != len)
        {
            warning("Entry %d too short, falling back to alternate strings", i);
            goto fail;
        }
    }

    file_close(&f);
    rb->splashf(HZ, "String table '%s' loaded", title_buf);
    return true;
fail:
    file_close(&f);
    return false;
}

void engine_init(struct Engine* e) {
    sys_init(e->sys, "Out Of This World");

    res_readEntries(&e->res);

    engine_loadStringTable(e);

    engine_loadFontFile(e);

    video_init(&e->video);

    vm_init(&e->vm);

    mixer_init(&e->mixer);

    player_init(&e->player);

    /* Init virtual machine, legacy way */
    vm_initForPart(&e->vm, GAME_PART_FIRST); // This game part is the protection screen */

    /*  Try to cheat here. You can jump anywhere but the VM crashes afterward. */
    /*  Starting somewhere is probably not enough, the variables and calls return are probably missing. */
    /* vm_initForPart(&e->vm, GAME_PART2); Skip protection screen and go directly to intro */
    /* vm_initForPart(&e->vm, GAME_PART3);  CRASH */
    /* vm_initForPart(&e->vm, GAME_PART4);  Start directly in jail but then crash */
    /* vm->initForPart(&e->vm, GAME_PART5);   CRASH */
    /* vm->initForPart(GAME_PART6);    Start in the battlechar but CRASH afteward */
    /* vm->initForPart(GAME_PART7); CRASH */
    /* vm->initForPart(GAME_PART8); CRASH */
    /* vm->initForPart(GAME_PART9);  Green screen not doing anything */
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
        e->vm._fastMode = !&e->vm._fastMode;
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
    rb->snprintf(buf, sz, "xworld_save.s%02d", slot);
}

void engine_saveGameState(struct Engine* e, uint8_t slot, const char *desc) {
    char stateFile[20];
    /* sizeof(char) is guaranteed to be 1 */
    engine_makeGameStateName(e, slot, stateFile, sizeof(stateFile));
    File f;
    file_create(&f, false);
    if (!file_open(&f, stateFile, e->_saveDir, "wb")) {
        warning("Unable to save state file '%s'", stateFile);
    } else {
        /* header */
        file_writeUint32BE(&f, SAVE_MAGIC);
        file_writeUint16BE(&f, CUR_VER);
        file_writeUint16BE(&f, 0);
        char hdrdesc[32];
        strncpy(hdrdesc, desc, sizeof(hdrdesc) - 1);
        file_write(&f, hdrdesc, sizeof(hdrdesc));
        /* contents */
        struct Serializer s;
        ser_create(&s, &f, SM_SAVE, e->res._memPtrStart, CUR_VER);
        vm_saveOrLoad(&e->vm, &s);
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

bool engine_loadGameState(struct Engine* e, uint8_t slot) {
    char stateFile[20];
    engine_makeGameStateName(e, slot, stateFile, 20);
    File f;
    file_create(&f, false);
    if (!file_open(&f, stateFile, e->_saveDir, "rb")) {
        debug(DBG_ENG, "Unable to open state file '%s'", stateFile);
        goto fail;
    } else {
        uint32_t id = file_readUint32BE(&f);
        if (id != SAVE_MAGIC) {
            debug(DBG_ENG, "Bad savegame format");
            goto fail;
        } else {
            /* mute */
            player_stop(&e->player);
            mixer_stopAll(&e->mixer);
            /* header */
            uint16_t ver = file_readUint16BE(&f);
            file_readUint16BE(&f);
            char hdrdesc[32];
            file_read(&f, hdrdesc, sizeof(hdrdesc));
            /* contents */
            struct Serializer s;
            ser_create(&s, &f, SM_LOAD, e->res._memPtrStart, ver);
            vm_saveOrLoad(&e->vm, &s);
            res_saveOrLoad(&e->res, &s);
            video_saveOrLoad(&e->video, &s);
            player_saveOrLoad(&e->player, &s);
            mixer_saveOrLoad(&e->mixer, &s);
        }
        if (file_ioErr(&f)) {
            debug(DBG_ENG, "I/O error when loading game state");
            goto fail;
        } else {
            debug(DBG_INFO, "Loaded state from slot %d", e->_stateSlot);
        }
    }
    file_close(&f);
    return true;
fail:
    file_close(&f);
    return false;
}

void engine_deleteGameState(struct Engine* e, uint8_t slot) {
    char stateFile[20];
    engine_makeGameStateName(e, slot, stateFile, 20);
    file_remove(stateFile, e->_saveDir);
}

const char* engine_getDataDir(struct Engine* e)
{
    return e->_dataDir;
}

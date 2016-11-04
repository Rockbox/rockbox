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
#include "vm.h"
#include "mixer.h"
#include "resource.h"
#include "video.h"
#include "serializer.h"
#include "sfxplayer.h"
#include "sys.h"
#include "parts.h"
#include "file.h"

static const uint16_t vm_frequenceTable[] = {
    0x0CFF, 0x0DC3, 0x0E91, 0x0F6F, 0x1056, 0x114E, 0x1259, 0x136C,
    0x149F, 0x15D9, 0x1726, 0x1888, 0x19FD, 0x1B86, 0x1D21, 0x1EDE,
    0x20AB, 0x229C, 0x24B3, 0x26D7, 0x293F, 0x2BB2, 0x2E4C, 0x3110,
    0x33FB, 0x370D, 0x3A43, 0x3DDF, 0x4157, 0x4538, 0x4998, 0x4DAE,
    0x5240, 0x5764, 0x5C9A, 0x61C8, 0x6793, 0x6E19, 0x7485, 0x7BBD
};

void vm_create(struct VirtualMachine* m, struct Mixer *mix,  struct Resource* res, struct SfxPlayer *ply, struct Video *vid, struct System *stub)
{
    m->res = res;
    m->video = vid;
    m->sys = stub;
    m->mixer = mix;
    m->player = ply;
}

void vm_init(struct VirtualMachine* m) {

    rb->memset(m->vmVariables, 0, sizeof(m->vmVariables));
    m->vmVariables[0x54] = 0x81;
    m->vmVariables[VM_VARIABLE_RANDOM_SEED] = *rb->current_tick % 0x10000;

    /* rawgl has these, but they don't seem to do anything */
    //m->vmVariables[0xBC] = 0x10;
    //m->vmVariables[0xC6] = 0x80;
    //m->vmVariables[0xF2] = 4000;
    //m->vmVariables[0xDC] = 33;

    m->_fastMode = false;
    m->player->_markVar = &m->vmVariables[VM_VARIABLE_MUS_MARK];
}

void vm_op_movConst(struct VirtualMachine* m) {
    uint8_t variableId = scriptPtr_fetchByte(&m->_scriptPtr);
    int16_t value = scriptPtr_fetchWord(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_movConst(0x%02X, %d)", variableId, value);
    m->vmVariables[variableId] = value;
}

void vm_op_mov(struct VirtualMachine* m) {
    uint8_t dstVariableId = scriptPtr_fetchByte(&m->_scriptPtr);
    uint8_t srcVariableId = scriptPtr_fetchByte(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_mov(0x%02X, 0x%02X)", dstVariableId, srcVariableId);
    m->vmVariables[dstVariableId] = m->vmVariables[srcVariableId];
}

void vm_op_add(struct VirtualMachine* m) {
    uint8_t dstVariableId = scriptPtr_fetchByte(&m->_scriptPtr);
    uint8_t srcVariableId = scriptPtr_fetchByte(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_add(0x%02X, 0x%02X)", dstVariableId, srcVariableId);
    m->vmVariables[dstVariableId] += m->vmVariables[srcVariableId];
}

void vm_op_addConst(struct VirtualMachine* m) {
    if (m->res->currentPartId == 0x3E86 && m->_scriptPtr.pc == m->res->segBytecode + 0x6D48) {
        //warning("vm_op_addConst() hack for non-stop looping gun sound bug");
        // the script 0x27 slot 0x17 doesn't stop the gun sound from looping, I
        // don't really know why ; for now, let's play the 'stopping sound' like
        // the other scripts do
        //  (0x6D43) jmp(0x6CE5)
        //  (0x6D46) break
        //  (0x6D47) VAR(6) += -50
        vm_snd_playSound(m, 0x5B, 1, 64, 1);
    }
    uint8_t variableId = scriptPtr_fetchByte(&m->_scriptPtr);
    int16_t value = scriptPtr_fetchWord(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_addConst(0x%02X, %d)", variableId, value);
    m->vmVariables[variableId] += value;
}

void vm_op_call(struct VirtualMachine* m) {

    uint16_t offset = scriptPtr_fetchWord(&m->_scriptPtr);
    uint8_t sp = m->_stackPtr;

    debug(DBG_VM, "vm_op_call(0x%X)", offset);
    m->_scriptStackCalls[sp] = m->_scriptPtr.pc - m->res->segBytecode;
    if (m->_stackPtr == 0xFF) {
        error("vm_op_call() ec=0x%X stack overflow", 0x8F);
    }
    ++m->_stackPtr;
    m->_scriptPtr.pc = m->res->segBytecode + offset ;
}

void vm_op_ret(struct VirtualMachine* m) {
    debug(DBG_VM, "vm_op_ret()");
    if (m->_stackPtr == 0) {
        error("vm_op_ret() ec=0x%X stack underflow", 0x8F);
    }
    --m->_stackPtr;
    uint8_t sp = m->_stackPtr;
    m->_scriptPtr.pc = m->res->segBytecode + m->_scriptStackCalls[sp];
}

void vm_op_pauseThread(struct VirtualMachine* m) {
    debug(DBG_VM, "vm_op_pauseThread()");
    m->gotoNextThread = true;
}

void vm_op_jmp(struct VirtualMachine* m) {
    uint16_t pcOffset = scriptPtr_fetchWord(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_jmp(0x%02X)", pcOffset);
    m->_scriptPtr.pc = m->res->segBytecode + pcOffset;
}

void vm_op_setSetVect(struct VirtualMachine* m) {
    uint8_t threadId = scriptPtr_fetchByte(&m->_scriptPtr);
    uint16_t pcOffsetRequested = scriptPtr_fetchWord(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_setSetVect(0x%X, 0x%X)", threadId, pcOffsetRequested);
    m->threadsData[REQUESTED_PC_OFFSET][threadId] = pcOffsetRequested;
}

void vm_op_jnz(struct VirtualMachine* m) {
    uint8_t i = scriptPtr_fetchByte(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_jnz(0x%02X)", i);
    --m->vmVariables[i];
    if (m->vmVariables[i] != 0) {
        vm_op_jmp(m);
    } else {
        scriptPtr_fetchWord(&m->_scriptPtr);
    }
}

#define BYPASS_PROTECTION
void vm_op_condJmp(struct VirtualMachine* m) {

    //debug(DBG_VM, "Jump : %X \n",m->_scriptPtr.pc-m->res->segBytecode);
//FCS Whoever wrote this is patching the bytecode on the fly. This is ballzy !!
#if 0
    if (m->res->currentPartId == GAME_PART_FIRST && m->_scriptPtr.pc == m->res->segBytecode + 0xCB9) {

        // (0x0CB8) condJmp(0x80, VAR(41), VAR(30), 0xCD3)
        *(m->_scriptPtr.pc + 0x00) = 0x81;
        *(m->_scriptPtr.pc + 0x03) = 0x0D;
        *(m->_scriptPtr.pc + 0x04) = 0x24;
        // (0x0D4E) condJmp(0x4, VAR(50), 6, 0xDBC)
        *(m->_scriptPtr.pc + 0x99) = 0x0D;
        *(m->_scriptPtr.pc + 0x9A) = 0x5A;
        debug(DBG_VM, "vm_op_condJmp() bypassing protection");
        debug(DBG_VM, "bytecode has been patched");

        //warning("bypassing protection");

        //vm_bypassProtection(m);
    }


#endif

    uint8_t opcode = scriptPtr_fetchByte(&m->_scriptPtr);
    uint8_t var = scriptPtr_fetchByte(&m->_scriptPtr);
    int16_t b = m->vmVariables[var];
    uint8_t c = scriptPtr_fetchByte(&m->_scriptPtr);
    int16_t a;

    if (opcode & 0x80) {
        a = m->vmVariables[c];
    } else if (opcode & 0x40) {
        a = c * 256 + scriptPtr_fetchByte(&m->_scriptPtr);
    } else {
        a = c;
    }
    debug(DBG_VM, "vm_op_condJmp(%d, 0x%02X, 0x%02X)", opcode, b, a);

    // Check if the conditional value is met.
    bool expr = false;
    switch (opcode & 7) {
    case 0:     // jz
        expr = (b == a);

#ifdef BYPASS_PROTECTION
        /* always succeed in code wheel verification */
        if (m->res->currentPartId == GAME_PART_FIRST && var == 0x29 && (opcode & 0x80) != 0) {

            m->vmVariables[0x29] = m->vmVariables[0x1E];
            m->vmVariables[0x2A] = m->vmVariables[0x1F];
            m->vmVariables[0x2B] = m->vmVariables[0x20];
            m->vmVariables[0x2C] = m->vmVariables[0x21];
            // counters
            m->vmVariables[0x32] = 6;
            m->vmVariables[0x64] = 20;
            expr = true;
            //warning("Script::op_condJmp() bypassing protection");
        }
#endif
        break;
    case 1: // jnz
        expr = (b != a);
        break;
    case 2: // jg
        expr = (b > a);
        break;
    case 3: // jge
        expr = (b >= a);
        break;
    case 4: // jl
        expr = (b < a);
        break;
    case 5: // jle
        expr = (b <= a);
        break;
    default:
        warning("vm_op_condJmp() invalid condition %d", (opcode & 7));
        break;
    }

    if (expr) {
        vm_op_jmp(m);
    } else {
        scriptPtr_fetchWord(&m->_scriptPtr);
    }

}

void vm_op_setPalette(struct VirtualMachine* m) {
    uint16_t paletteId = scriptPtr_fetchWord(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_changePalette(%d)", paletteId);
    m->video->paletteIdRequested = paletteId >> 8;
}

void vm_op_resetThread(struct VirtualMachine* m) {

    uint8_t threadId = scriptPtr_fetchByte(&m->_scriptPtr);
    uint8_t i =        scriptPtr_fetchByte(&m->_scriptPtr);

    // FCS: WTF, this is cryptic as hell !!
    // int8_t n = (i & 0x3F) - threadId;  //0x3F = 0011 1111
    // The following is so much clearer

    //Make sure i is within [0-VM_NUM_THREADS-1]
    i = i & (VM_NUM_THREADS - 1) ;
    int8_t n = i - threadId;

    if (n < 0) {
        warning("vm_op_m->resetThread() ec=0x%X (n < 0)", 0x880);
        return;
    }
    ++n;
    uint8_t a = scriptPtr_fetchByte(&m->_scriptPtr);

    debug(DBG_VM, "vm_op_m->resetThread(%d, %d, %d)", threadId, i, a);

    if (a == 2) {
        uint16_t *p = &m->threadsData[REQUESTED_PC_OFFSET][threadId];
        while (n--) {
            *p++ = 0xFFFE;
        }
    } else if (a < 2) {
        uint8_t *p = &m->vmIsChannelActive[REQUESTED_STATE][threadId];
        while (n--) {
            *p++ = a;
        }
    }
}

void vm_op_selectVideoPage(struct VirtualMachine* m) {
    uint8_t frameBufferId = scriptPtr_fetchByte(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_selectVideoPage(%d)", frameBufferId);
    video_changePagePtr1(m->video, frameBufferId);
}

void vm_op_fillVideoPage(struct VirtualMachine* m) {
    uint8_t pageId = scriptPtr_fetchByte(&m->_scriptPtr);
    uint8_t color = scriptPtr_fetchByte(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_fillVideoPage(%d, %d)", pageId, color);
    video_fillPage(m->video, pageId, color);
}

void vm_op_copyVideoPage(struct VirtualMachine* m) {
    uint8_t srcPageId = scriptPtr_fetchByte(&m->_scriptPtr);
    uint8_t dstPageId = scriptPtr_fetchByte(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_copyVideoPage(%d, %d)", srcPageId, dstPageId);
    video_copyPage(m->video, srcPageId, dstPageId, m->vmVariables[VM_VARIABLE_SCROLL_Y]);
}


static uint32_t lastTimeStamp = 0;
void vm_op_blitFramebuffer(struct VirtualMachine* m) {

    uint8_t pageId = scriptPtr_fetchByte(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_blitFramebuffer(%d)", pageId);
    vm_inp_handleSpecialKeys(m);

    /* Nasty hack....was this present in the original assembly  ??!! */
    if (m->res->currentPartId == GAME_PART_FIRST && m->vmVariables[0x67] == 1)
        m->vmVariables[0xDC] = 0x21;

    if (!m->_fastMode) {

        int32_t delay = sys_getTimeStamp(m->sys) - lastTimeStamp;
        int32_t timeToSleep = m->vmVariables[VM_VARIABLE_PAUSE_SLICES] * 20 - delay;

        /* The bytecode will set m->vmVariables[VM_VARIABLE_PAUSE_SLICES] from 1 to 5 */
        /* The virtual machine hence indicates how long the image should be displayed. */

        if (timeToSleep > 0)
        {
            sys_sleep(m->sys, timeToSleep);
        }

        lastTimeStamp = sys_getTimeStamp(m->sys);
    }

    /* WTF ? */
    m->vmVariables[0xF7] = 0;

    video_updateDisplay(m->video, pageId);
}

void vm_op_killThread(struct VirtualMachine* m) {
    debug(DBG_VM, "vm_op_killThread()");
    m->_scriptPtr.pc = m->res->segBytecode + 0xFFFF;
    m->gotoNextThread = true;
}

void vm_op_drawString(struct VirtualMachine* m) {
    uint16_t stringId = scriptPtr_fetchWord(&m->_scriptPtr);
    uint16_t x = scriptPtr_fetchByte(&m->_scriptPtr);
    uint16_t y = scriptPtr_fetchByte(&m->_scriptPtr);
    uint16_t color = scriptPtr_fetchByte(&m->_scriptPtr);

    debug(DBG_VM, "vm_op_drawString(0x%03X, %d, %d, %d)", stringId, x, y, color);

    video_drawString(m->video, color, x, y, stringId);
}

void vm_op_sub(struct VirtualMachine* m) {
    uint8_t i = scriptPtr_fetchByte(&m->_scriptPtr);
    uint8_t j = scriptPtr_fetchByte(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_sub(0x%02X, 0x%02X)", i, j);
    m->vmVariables[i] -= m->vmVariables[j];
}

void vm_op_and(struct VirtualMachine* m) {
    uint8_t variableId = scriptPtr_fetchByte(&m->_scriptPtr);
    uint16_t n = scriptPtr_fetchWord(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_and(0x%02X, %d)", variableId, n);
    m->vmVariables[variableId] = (uint16_t)m->vmVariables[variableId] & n;
}

void vm_op_or(struct VirtualMachine* m) {
    uint8_t variableId = scriptPtr_fetchByte(&m->_scriptPtr);
    uint16_t value = scriptPtr_fetchWord(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_or(0x%02X, %d)", variableId, value);
    m->vmVariables[variableId] = (uint16_t)m->vmVariables[variableId] | value;
}

void vm_op_shl(struct VirtualMachine* m) {
    uint8_t variableId = scriptPtr_fetchByte(&m->_scriptPtr);
    uint16_t leftShiftValue = scriptPtr_fetchWord(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_shl(0x%02X, %d)", variableId, leftShiftValue);
    m->vmVariables[variableId] = (uint16_t)m->vmVariables[variableId] << leftShiftValue;
}

void vm_op_shr(struct VirtualMachine* m) {
    uint8_t variableId = scriptPtr_fetchByte(&m->_scriptPtr);
    uint16_t rightShiftValue = scriptPtr_fetchWord(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_shr(0x%02X, %d)", variableId, rightShiftValue);
    m->vmVariables[variableId] = (uint16_t)m->vmVariables[variableId] >> rightShiftValue;
}

void vm_op_playSound(struct VirtualMachine* m) {
    uint16_t resourceId = scriptPtr_fetchWord(&m->_scriptPtr);
    uint8_t freq = scriptPtr_fetchByte(&m->_scriptPtr);
    uint8_t vol = scriptPtr_fetchByte(&m->_scriptPtr);
    uint8_t channel = scriptPtr_fetchByte(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_playSound(0x%X, %d, %d, %d)", resourceId, freq, vol, channel);
    vm_snd_playSound(m, resourceId, freq, vol, channel);
}

void vm_op_updateMemList(struct VirtualMachine* m) {

    uint16_t resourceId = scriptPtr_fetchWord(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_updateMemList(%d)", resourceId);

    if (resourceId == 0) {
        player_stop(m->player);
        mixer_stopAll(m->mixer);
        res_invalidateRes(m->res);
    } else {
        res_loadPartsOrMemoryEntry(m->res, resourceId);
    }
}

void vm_op_playMusic(struct VirtualMachine* m) {
    uint16_t resNum = scriptPtr_fetchWord(&m->_scriptPtr);
    uint16_t delay = scriptPtr_fetchWord(&m->_scriptPtr);
    uint8_t pos = scriptPtr_fetchByte(&m->_scriptPtr);
    debug(DBG_VM, "vm_op_playMusic(0x%X, %d, %d)", resNum, delay, pos);
    vm_snd_playMusic(m, resNum, delay, pos);
}

void vm_initForPart(struct VirtualMachine* m, uint16_t partId) {

    player_stop(m->player);
    mixer_stopAll(m->mixer);

    /* WTF is that ? */
    m->vmVariables[0xE4] = 0x14;

    res_setupPart(m->res, partId);

    /* Set all thread to inactive (pc at 0xFFFF or 0xFFFE ) */
    rb->memset((uint8_t *)m->threadsData, 0xFF, sizeof(m->threadsData));

    rb->memset((uint8_t *)m->vmIsChannelActive, 0, sizeof(m->vmIsChannelActive));

    int firstThreadId = 0;
    m->threadsData[PC_OFFSET][firstThreadId] = 0;
}

/*
  This is called every frames in the infinite loop.
*/
void vm_checkThreadRequests(struct VirtualMachine* m) {

    /* Check if a part switch has been requested. */
    if (m->res->requestedNextPart != 0) {
        vm_initForPart(m, m->res->requestedNextPart);
        m->res->requestedNextPart = 0;
    }


    /* Check if a state update has been requested for any thread during the previous VM execution: */
    /*      - Pause */
    /*      - Jump */

    /* JUMP: */
    /* Note: If a jump has been requested, the jump destination is stored */
    /* in m->threadsData[REQUESTED_PC_OFFSET]. Otherwise m->threadsData[REQUESTED_PC_OFFSET] == 0xFFFF */

    /* PAUSE: */
    /* Note: If a pause has been requested it is stored in  m->vmIsChannelActive[REQUESTED_STATE][i] */

    for (int threadId = 0; threadId < VM_NUM_THREADS; threadId++) {

        m->vmIsChannelActive[CURR_STATE][threadId] = m->vmIsChannelActive[REQUESTED_STATE][threadId];

        uint16_t n = m->threadsData[REQUESTED_PC_OFFSET][threadId];

        if (n != VM_NO_SETVEC_REQUESTED) {

            m->threadsData[PC_OFFSET][threadId] = (n == 0xFFFE) ? VM_INACTIVE_THREAD : n;
            m->threadsData[REQUESTED_PC_OFFSET][threadId] = VM_NO_SETVEC_REQUESTED;
        }
    }
}

void vm_hostFrame(struct VirtualMachine* m) {

    /* Run the Virtual Machine for every active threads (one vm frame). */
    /* Inactive threads are marked with a thread instruction pointer set to 0xFFFF (VM_INACTIVE_THREAD). */
    /* A thread must feature a break opcode so the interpreter can move to the next thread. */

    for (int threadId = 0; threadId < VM_NUM_THREADS; threadId++) {

        if (m->vmIsChannelActive[CURR_STATE][threadId])
            continue;

        uint16_t n = m->threadsData[PC_OFFSET][threadId];

        if (n != VM_INACTIVE_THREAD) {

            /* Set the script pointer to the right location. */
            /* script pc is used in executeThread in order */
            /* to get the next opcode. */
            m->_scriptPtr.pc = m->res->segBytecode + n;
            m->_stackPtr = 0;

            m->gotoNextThread = false;
            debug(DBG_VM, "vm_hostFrame() i=0x%02X n=0x%02X *p=0x%02X", threadId, n, *m->_scriptPtr.pc);
            vm_executeThread(m);

            /* Since .pc is going to be modified by this next loop iteration, we need to save it. */
            m->threadsData[PC_OFFSET][threadId] = m->_scriptPtr.pc - m->res->segBytecode;


            debug(DBG_VM, "vm_hostFrame() i=0x%02X pos=0x%X", threadId, m->threadsData[PC_OFFSET][threadId]);
            if (m->sys->input.quit) {
                break;
            }
        }
    }
}

#define COLOR_BLACK 0xFF
#define DEFAULT_ZOOM 0x40


void vm_executeThread(struct VirtualMachine* m) {

    while (!m->gotoNextThread) {
        uint8_t opcode = scriptPtr_fetchByte(&m->_scriptPtr);

        /* 1000 0000 is set */
        if (opcode & 0x80)
        {
            uint16_t off = ((opcode << 8) | scriptPtr_fetchByte(&m->_scriptPtr)) * 2;
            m->res->_useSegVideo2 = false;
            int16_t x = scriptPtr_fetchByte(&m->_scriptPtr);
            int16_t y = scriptPtr_fetchByte(&m->_scriptPtr);
            int16_t h = y - 199;
            if (h > 0) {
                y = 199;
                x += h;
            }
            debug(DBG_VIDEO, "vid_opcd_0x80 : opcode=0x%X off=0x%X x=%d y=%d", opcode, off, x, y);

            /* This switch the polygon database to "cinematic" and probably draws a black polygon */
            /* over all the screen. */
            video_setDataBuffer(m->video, m->res->segCinematic, off);
            struct Point temp;
            temp.x = x;
            temp.y = y;
            video_readAndDrawPolygon(m->video, COLOR_BLACK, DEFAULT_ZOOM, &temp);

            continue;
        }

        /* 0100 0000 is set */
        if (opcode & 0x40)
        {
            int16_t x, y;
            uint16_t off = scriptPtr_fetchWord(&m->_scriptPtr) * 2;
            x = scriptPtr_fetchByte(&m->_scriptPtr);

            m->res->_useSegVideo2 = false;

            if (!(opcode & 0x20))
            {
                if (!(opcode & 0x10))  /* 0001 0000 is set */
                {
                    x = (x << 8) | scriptPtr_fetchByte(&m->_scriptPtr);
                } else {
                    x = m->vmVariables[x];
                }
            }
            else
            {
                if (opcode & 0x10) { /* 0001 0000 is set */
                    x += 0x100;
                }
            }

            y = scriptPtr_fetchByte(&m->_scriptPtr);

            if (!(opcode & 8))  /* 0000 1000 is set */
            {
                if (!(opcode & 4)) { /* 0000 0100 is set */
                    y = (y << 8) | scriptPtr_fetchByte(&m->_scriptPtr);
                } else {
                    y = m->vmVariables[y];
                }
            }

            uint16_t zoom = scriptPtr_fetchByte(&m->_scriptPtr);

            if (!(opcode & 2))  /* 0000 0010 is set */
            {
                if (!(opcode & 1)) /* 0000 0001 is set */
                {
                    --m->_scriptPtr.pc;
                    zoom = 0x40;
                }
                else
                {
                    zoom = m->vmVariables[zoom];
                }
            }
            else
            {

                if (opcode & 1) { /* 0000 0001 is set */
                    m->res->_useSegVideo2 = true;
                    --m->_scriptPtr.pc;
                    zoom = 0x40;
                }
            }
            debug(DBG_VIDEO, "vid_opcd_0x40 : off=0x%X x=%d y=%d", off, x, y);
            video_setDataBuffer(m->video, m->res->_useSegVideo2 ? m->res->_segVideo2 : m->res->segCinematic, off);
            struct Point temp;
            temp.x = x;
            temp.y = y;
            video_readAndDrawPolygon(m->video, 0xFF, zoom, &temp);

            continue;
        }


        if (opcode > 0x1A)
        {
            error("vm_executeThread() ec=0x%X invalid opcode=0x%X", 0xFFF, opcode);
        }
        else
        {
            (vm_opcodeTable[opcode])(m);
        }
    }
}

void vm_inp_updatePlayer(struct VirtualMachine* m) {

    sys_processEvents(m->sys);

    if (m->res->currentPartId == 0x3E89) {
        char c = m->sys->input.lastChar;
        if (c == 8 || /*c == 0xD |*/ c == 0 || (c >= 'a' && c <= 'z')) {
            m->vmVariables[VM_VARIABLE_LAST_KEYCHAR] = c & ~0x20;
            m->sys->input.lastChar = 0;
        }
    }

    int16_t lr   = 0;
    int16_t mask = 0;
    int16_t ud   = 0;

    if (m->sys->input.dirMask & DIR_RIGHT) {
        lr = 1;
        mask |= 1;
    }
    if (m->sys->input.dirMask & DIR_LEFT) {
        lr = -1;
        mask |= 2;
    }
    if (m->sys->input.dirMask & DIR_DOWN) {
        ud = 1;
        mask |= 4;
    }

    m->vmVariables[VM_VARIABLE_HERO_POS_UP_DOWN] = ud;

    if (m->sys->input.dirMask & DIR_UP) {
        m->vmVariables[VM_VARIABLE_HERO_POS_UP_DOWN] = -1;
    }

    if (m->sys->input.dirMask & DIR_UP) { /* inpJump */
        ud = -1;
        mask |= 8;
    }

    m->vmVariables[VM_VARIABLE_HERO_POS_JUMP_DOWN] = ud;
    m->vmVariables[VM_VARIABLE_HERO_POS_LEFT_RIGHT] = lr;
    m->vmVariables[VM_VARIABLE_HERO_POS_MASK] = mask;
    int16_t button = 0;

    if (m->sys->input.button) {
        button = 1;
        mask |= 0x80;
    }

    m->vmVariables[VM_VARIABLE_HERO_ACTION] = button;
    m->vmVariables[VM_VARIABLE_HERO_ACTION_POS_MASK] = mask;
}

void vm_inp_handleSpecialKeys(struct VirtualMachine* m) {

    if (m->sys->input.pause) {

        if (m->res->currentPartId != GAME_PART1 && m->res->currentPartId != GAME_PART2) {
            m->sys->input.pause = false;
            while (!m->sys->input.pause) {
                sys_processEvents(m->sys);
                sys_sleep(m->sys, 200);
            }
        }
        m->sys->input.pause = false;
    }

    if (m->sys->input.code) {
        m->sys->input.code = false;
        if (m->res->currentPartId != GAME_PART_LAST && m->res->currentPartId != GAME_PART_FIRST) {
            m->res->requestedNextPart = GAME_PART_LAST;
        }
    }

    /* User has inputted a bad code, the "ERROR" screen is showing */
    if (m->vmVariables[0xC9] == 1) {
        debug(DBG_VM, "vm_inp_handleSpecialKeys() unhandled case (m->vmVariables[0xC9] == 1)");
    }

}

void vm_snd_playSound(struct VirtualMachine* m, uint16_t resNum, uint8_t freq, uint8_t vol, uint8_t channel) {

    debug(DBG_SND, "snd_playSound(0x%X, %d, %d, %d)", resNum, freq, vol, channel);

    struct MemEntry *me = &m->res->_memList[resNum];

    if (me->state != MEMENTRY_STATE_LOADED)
        return;


    if (vol == 0) {
        mixer_stopChannel(m->mixer, channel);
    } else {
        struct MixerChunk mc;
        rb->memset(&mc, 0, sizeof(mc));
        mc.data = me->bufPtr + 8; /* skip header */
        mc.len = READ_BE_UINT16(me->bufPtr) * 2;
        mc.loopLen = READ_BE_UINT16(me->bufPtr + 2) * 2;
        if (mc.loopLen != 0) {
            mc.loopPos = mc.len;
        }
        assert(freq < 40);
        mixer_playChannel(m->mixer, channel & 3, &mc, vm_frequenceTable[freq], MIN(vol, 0x3F));
    }

}

void vm_snd_playMusic(struct VirtualMachine* m, uint16_t resNum, uint16_t delay, uint8_t pos) {

    debug(DBG_SND, "snd_playMusic(0x%X, %d, %d)", resNum, delay, pos);

    if (resNum != 0) {
        player_loadSfxModule(m->player, resNum, delay, pos);
        player_start(m->player);
    } else if (delay != 0) {
        player_setEventsDelay(m->player, delay);
    } else {
        player_stop(m->player);
    }
}

void vm_saveOrLoad(struct VirtualMachine* m, struct Serializer *ser) {
    struct Entry entries[] = {
        SE_ARRAY(m->vmVariables, 0x100, SES_INT16, VER(1)),
        SE_ARRAY(m->_scriptStackCalls, 0x100, SES_INT16, VER(1)),
        SE_ARRAY(m->threadsData, 0x40 * 2, SES_INT16, VER(1)),
        SE_ARRAY(m->vmIsChannelActive, 0x40 * 2, SES_INT8, VER(1)),
        SE_END()
    };
    ser_saveOrLoadEntries(ser, entries);
}

void vm_bypassProtection(struct VirtualMachine* m)
{
    File f;
    file_create(&f, true);
    if (!file_open(&f, "bank0e", res_getDataDir(m->res), "rb")) {
        warning("Unable to bypass protection: add bank0e file to datadir");
    } else {
        struct Serializer s;
        ser_create(&s, &f, SM_LOAD, m->res->_memPtrStart, 2);
        vm_saveOrLoad(m, &s);
        res_saveOrLoad(m->res, &s);
        video_saveOrLoad(m->video, &s);
        player_saveOrLoad(m->player, &s);
        mixer_saveOrLoad(m->mixer, &s);
    }
    file_close(&f);
}

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

#include <ctime>
#include "vm.h"
#include "mixer.h"
#include "resource.h"
#include "video.h"
#include "serializer.h"
#include "sfxplayer.h"
#include "sys.h"
#include "parts.h"
#include "file.h"

void vm_VirtualMachine(struct VirtualMachine* m, Mixer *mix, Resource *resParameter, SfxPlayer *ply, Video *vid, System *stub)
    : mixer(mix), res(resParameter), struct VirtualMachine* myer(ply), video(vid), sys(stub) {
}

void vm_init(struct VirtualMachine* m) {

    memset(m->vmVariables, 0, sizeof(m->vmVariables));
    m->vmVariables[0x54] = 0x81;
    m->vmVariables[VM_VARIABLE_RANDOM_SEED] = time(0);

    m->_fastMode = false;
    player->_markVar = &m->vmVariables[VM_VARIABLE_MUS_MARK];
}

void vm_op_movConst(struct VirtualMachine* m) {
    uint8_t variableId = m->_scriptPtr.fetchByte();
    int16_t value = m->_scriptPtr.fetchWord();
    debug(DBG_VM, "vm_op_movConst(0x%02X, %d)", variableId, value);
    m->vmVariables[variableId] = value;
}

void vm_op_mov(struct VirtualMachine* m) {
    uint8_t dstVariableId = m->_scriptPtr.fetchByte();
    uint8_t srcVariableId = m->_scriptPtr.fetchByte();
    debug(DBG_VM, "vm_op_mov(0x%02X, 0x%02X)", dstVariableId, srcVariableId);
    m->vmVariables[dstVariableId] = m->vmVariables[srcVariableId];
}

void vm_op_add(struct VirtualMachine* m) {
    uint8_t dstVariableId = m->_scriptPtr.fetchByte();
    uint8_t srcVariableId = m->_scriptPtr.fetchByte();
    debug(DBG_VM, "vm_op_add(0x%02X, 0x%02X)", dstVariableId, srcVariableId);
    m->vmVariables[dstVariableId] += m->vmVariables[srcVariableId];
}

void vm_op_addConst(struct VirtualMachine* m) {
    if (res->currentPartId == 0x3E86 && m->_scriptPtr.pc == res->segBytecode + 0x6D48) {
        warning("vm_op_addConst() hack for non-stop looping gun sound bug");
        // the script 0x27 slot 0x17 doesn't stop the gun sound from looping, I
        // don't really know why ; for now, let's play the 'stopping sound' like
        // the other scripts do
        //  (0x6D43) jmp(0x6CE5)
        //  (0x6D46) break
        //  (0x6D47) VAR(6) += -50
        snd_playSound(0x5B, 1, 64, 1);
    }
    uint8_t variableId = m->_scriptPtr.fetchByte();
    int16_t value = m->_scriptPtr.fetchWord();
    debug(DBG_VM, "vm_op_addConst(0x%02X, %d)", variableId, value);
    m->vmVariables[variableId] += value;
}

void vm_op_call(struct VirtualMachine* m) {

    uint16_t offset = m->_scriptPtr.fetchWord();
    uint8_t sp = _stackPtr;

    debug(DBG_VM, "vm_op_call(0x%X)", offset);
    _scriptStackCalls[sp] = m->_scriptPtr.pc - res->segBytecode;
    if (_stackPtr == 0xFF) {
        error("vm_op_call() ec=0x%X stack overflow", 0x8F);
    }
    ++_stackPtr;
    m->_scriptPtr.pc = res->segBytecode + offset ;
}

void vm_op_ret(struct VirtualMachine* m) {
    debug(DBG_VM, "vm_op_ret()");
    if (_stackPtr == 0) {
        error("vm_op_ret() ec=0x%X stack underflow", 0x8F);
    }
    --_stackPtr;
    uint8_t sp = _stackPtr;
    m->_scriptPtr.pc = res->segBytecode + _scriptStackCalls[sp];
}

void vm_op_pauseThread(struct VirtualMachine* m) {
    debug(DBG_VM, "vm_op_pauseThread()");
    gotoNextThread = true;
}

void vm_op_jmp(struct VirtualMachine* m) {
    uint16_t pcOffset = m->_scriptPtr.fetchWord();
    debug(DBG_VM, "vm_op_jmp(0x%02X)", pcOffset);
    m->_scriptPtr.pc = res->segBytecode + pcOffset;
}

void vm_op_setSetVect(struct VirtualMachine* m) {
    uint8_t threadId = m->_scriptPtr.fetchByte();
    uint16_t pcOffsetRequested = m->_scriptPtr.fetchWord();
    debug(DBG_VM, "vm_op_setSetVect(0x%X, 0x%X)", threadId,pcOffsetRequested);
    threadsData[REQUESTED_PC_OFFSET][threadId] = pcOffsetRequested;
}

void vm_op_jnz(struct VirtualMachine* m) {
    uint8_t i = m->_scriptPtr.fetchByte();
    debug(DBG_VM, "vm_op_jnz(0x%02X)", i);
    --m->vmVariables[i];
    if (m->vmVariables[i] != 0) {
        op_jmp();
    } else {
        m->_scriptPtr.fetchWord();
    }
}

#define BYPASS_PROTECTION
void vm_op_condJmp(struct VirtualMachine* m) {

    //printf("Jump : %X \n",m->_scriptPtr.pc-res->segBytecode);
//FCS Whoever wrote this is patching the bytecode on the fly. This is ballzy !!
#ifdef BYPASS_PROTECTION

    if (res->currentPartId == GAME_PART_FIRST && m->_scriptPtr.pc == res->segBytecode + 0xCB9) {

        // (0x0CB8) condJmp(0x80, VAR(41), VAR(30), 0xCD3)
        *(m->_scriptPtr.pc + 0x00) = 0x81;
        *(m->_scriptPtr.pc + 0x03) = 0x0D;
        *(m->_scriptPtr.pc + 0x04) = 0x24;
        // (0x0D4E) condJmp(0x4, VAR(50), 6, 0xDBC)
        *(m->_scriptPtr.pc + 0x99) = 0x0D;
        *(m->_scriptPtr.pc + 0x9A) = 0x5A;
        printf("vm_op_condJmp() bypassing protection");
        printf("bytecode has been patched/n");

        //this->bypassProtection() ;
    }


#endif

    uint8_t opcode = m->_scriptPtr.fetchByte();
    int16_t b = m->vmVariables[m->_scriptPtr.fetchByte()];
    uint8_t c = m->_scriptPtr.fetchByte();
    int16_t a;

    if (opcode & 0x80) {
        a = m->vmVariables[c];
    } else if (opcode & 0x40) {
        a = c * 256 + m->_scriptPtr.fetchByte();
    } else {
        a = c;
    }
    debug(DBG_VM, "vm_op_condJmp(%d, 0x%02X, 0x%02X)", opcode, b, a);

    // Check if the conditional value is met.
    bool expr = false;
    switch (opcode & 7) {
    case 0:	// jz
        expr = (b == a);
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
        op_jmp();
    } else {
        m->_scriptPtr.fetchWord();
    }

}

void vm_op_setPalette(struct VirtualMachine* m) {
    uint16_t paletteId = m->_scriptPtr.fetchWord();
    debug(DBG_VM, "vm_op_changePalette(%d)", paletteId);
    video->paletteIdRequested = paletteId >> 8;
}

void vm_op_resetThread(struct VirtualMachine* m) {

    uint8_t threadId = m->_scriptPtr.fetchByte();
    uint8_t i =        m->_scriptPtr.fetchByte();

    // FCS: WTF, this is cryptic as hell !!
    //int8_t n = (i & 0x3F) - threadId;  //0x3F = 0011 1111
    // The following is so much clearer

    //Make sure i within [0-VM_NUM_THREADS-1]
    i = i & (VM_NUM_THREADS-1) ;
    int8_t n = i - threadId;

    if (n < 0) {
        warning("vm_op_resetThread() ec=0x%X (n < 0)", 0x880);
        return;
    }
    ++n;
    uint8_t a = m->_scriptPtr.fetchByte();

    debug(DBG_VM, "vm_op_resetThread(%d, %d, %d)", threadId, i, a);

    if (a == 2) {
        uint16_t *p = &threadsData[REQUESTED_PC_OFFSET][threadId];
        while (n--) {
            *p++ = 0xFFFE;
        }
    } else if (a < 2) {
        uint8_t *p = &vmIsChannelActive[REQUESTED_STATE][threadId];
        while (n--) {
            *p++ = a;
        }
    }
}

void vm_op_selectVideoPage(struct VirtualMachine* m) {
    uint8_t frameBufferId = m->_scriptPtr.fetchByte();
    debug(DBG_VM, "vm_op_selectVideoPage(%d)", frameBufferId);
    video->changePagePtr1(frameBufferId);
}

void vm_op_fillVideoPage(struct VirtualMachine* m) {
    uint8_t pageId = m->_scriptPtr.fetchByte();
    uint8_t color = m->_scriptPtr.fetchByte();
    debug(DBG_VM, "vm_op_fillVideoPage(%d, %d)", pageId, color);
    video->fillPage(pageId, color);
}

void vm_op_copyVideoPage(struct VirtualMachine* m) {
    uint8_t srcPageId = m->_scriptPtr.fetchByte();
    uint8_t dstPageId = m->_scriptPtr.fetchByte();
    debug(DBG_VM, "vm_op_copyVideoPage(%d, %d)", srcPageId, dstPageId);
    video->copyPage(srcPageId, dstPageId, m->vmVariables[VM_VARIABLE_SCROLL_Y]);
}


uint32_t lastTimeStamp = 0;
void vm_op_blitFramebuffer(struct VirtualMachine* m) {

    uint8_t pageId = m->_scriptPtr.fetchByte();
    debug(DBG_VM, "vm_op_blitFramebuffer(%d)", pageId);
    inp_handleSpecialKeys();

    //Nasty hack....was this present in the original assembly  ??!!
    if (res->currentPartId == GAME_PART_FIRST && m->vmVariables[0x67] == 1)
        m->vmVariables[0xDC] = 0x21;

    if (!m->_fastMode) {

        int32_t delay = sys->getTimeStamp() - lastTimeStamp;
        int32_t timeToSleep = m->vmVariables[VM_VARIABLE_PAUSE_SLICES] * 20 - delay;

        // The bytecode will set m->vmVariables[VM_VARIABLE_PAUSE_SLICES] from 1 to 5
        // The virtual machine hence indicate how long the image should be displayed.

        //printf("m->vmVariables[VM_VARIABLE_PAUSE_SLICES]=%d\n",m->vmVariables[VM_VARIABLE_PAUSE_SLICES]);


        if (timeToSleep > 0)
        {
            //	printf("Sleeping for=%d\n",timeToSleep);
            sys->sleep(timeToSleep);
        }

        lastTimeStamp = sys->getTimeStamp();
    }

    //WTF ?
    m->vmVariables[0xF7] = 0;

    video->updateDisplay(pageId);
}

void vm_op_killThread(struct VirtualMachine* m) {
    debug(DBG_VM, "vm_op_killThread()");
    m->_scriptPtr.pc = res->segBytecode + 0xFFFF;
    gotoNextThread = true;
}

void vm_op_drawString(struct VirtualMachine* m) {
    uint16_t stringId = m->_scriptPtr.fetchWord();
    uint16_t x = m->_scriptPtr.fetchByte();
    uint16_t y = m->_scriptPtr.fetchByte();
    uint16_t color = m->_scriptPtr.fetchByte();

    debug(DBG_VM, "vm_op_drawString(0x%03X, %d, %d, %d)", stringId, x, y, color);

    video->drawString(color, x, y, stringId);
}

void vm_op_sub(struct VirtualMachine* m) {
    uint8_t i = m->_scriptPtr.fetchByte();
    uint8_t j = m->_scriptPtr.fetchByte();
    debug(DBG_VM, "vm_op_sub(0x%02X, 0x%02X)", i, j);
    m->vmVariables[i] -= m->vmVariables[j];
}

void vm_op_and(struct VirtualMachine* m) {
    uint8_t variableId = m->_scriptPtr.fetchByte();
    uint16_t n = m->_scriptPtr.fetchWord();
    debug(DBG_VM, "vm_op_and(0x%02X, %d)", variableId, n);
    m->vmVariables[variableId] = (uint16_t)m->vmVariables[variableId] & n;
}

void vm_op_or(struct VirtualMachine* m) {
    uint8_t variableId = m->_scriptPtr.fetchByte();
    uint16_t value = m->_scriptPtr.fetchWord();
    debug(DBG_VM, "vm_op_or(0x%02X, %d)", variableId, value);
    m->vmVariables[variableId] = (uint16_t)m->vmVariables[variableId] | value;
}

void vm_op_shl(struct VirtualMachine* m) {
    uint8_t variableId = m->_scriptPtr.fetchByte();
    uint16_t leftShiftValue = m->_scriptPtr.fetchWord();
    debug(DBG_VM, "vm_op_shl(0x%02X, %d)", variableId, leftShiftValue);
    m->vmVariables[variableId] = (uint16_t)m->vmVariables[variableId] << leftShiftValue;
}

void vm_op_shr(struct VirtualMachine* m) {
    uint8_t variableId = m->_scriptPtr.fetchByte();
    uint16_t rightShiftValue = m->_scriptPtr.fetchWord();
    debug(DBG_VM, "vm_op_shr(0x%02X, %d)", variableId, rightShiftValue);
    m->vmVariables[variableId] = (uint16_t)m->vmVariables[variableId] >> rightShiftValue;
}

void vm_op_playSound(struct VirtualMachine* m) {
    uint16_t resourceId = m->_scriptPtr.fetchWord();
    uint8_t freq = m->_scriptPtr.fetchByte();
    uint8_t vol = m->_scriptPtr.fetchByte();
    uint8_t channel = m->_scriptPtr.fetchByte();
    debug(DBG_VM, "vm_op_playSound(0x%X, %d, %d, %d)", resourceId, freq, vol, channel);
    snd_playSound(resourceId, freq, vol, channel);
}

void vm_op_updateMemList(struct VirtualMachine* m) {

    uint16_t resourceId = m->_scriptPtr.fetchWord();
    debug(DBG_VM, "vm_op_updateMemList(%d)", resourceId);

    if (resourceId == 0) {
        player->stop();
        mixer->stopAll();
        res->invalidateRes();
    } else {
        res->loadPartsOrMemoryEntry(resourceId);
    }
}

void vm_op_playMusic(struct VirtualMachine* m) {
    uint16_t resNum = m->_scriptPtr.fetchWord();
    uint16_t delay = m->_scriptPtr.fetchWord();
    uint8_t pos = m->_scriptPtr.fetchByte();
    debug(DBG_VM, "vm_op_playMusic(0x%X, %d, %d)", resNum, delay, pos);
    snd_playMusic(resNum, delay, pos);
}

void vm_initForPart(struct VirtualMachine* m, uint16_t partId) {

    player->stop();
    mixer->stopAll();

    //WTF is that ?
    m->vmVariables[0xE4] = 0x14;

    res->setupPart(partId);

    //Set all thread to inactive (pc at 0xFFFF or 0xFFFE )
    memset((uint8_t *)threadsData, 0xFF, sizeof(threadsData));


    memset((uint8_t *)vmIsChannelActive, 0, sizeof(vmIsChannelActive));

    int firstThreadId = 0;
    threadsData[PC_OFFSET][firstThreadId] = 0;
}

/*
  This is called every frames in the infinite loop.
*/
void vm_checkThreadRequests(struct VirtualMachine* m) {

    //Check if a part switch has been requested.
    if (res->requestedNextPart != 0) {
        initForPart(res->requestedNextPart);
        res->requestedNextPart = 0;
    }


    // Check if a state update has been requested for any thread during the previous VM execution:
    //      - Pause
    //      - Jump

    // JUMP:
    // Note: If a jump has been requested, the jump destination is stored
    // in threadsData[REQUESTED_PC_OFFSET]. Otherwise threadsData[REQUESTED_PC_OFFSET] == 0xFFFF

    // PAUSE:
    // Note: If a pause has been requested it is stored in  vmIsChannelActive[REQUESTED_STATE][i]

    for (int threadId = 0; threadId < VM_NUM_THREADS; threadId++) {

        vmIsChannelActive[CURR_STATE][threadId] = vmIsChannelActive[REQUESTED_STATE][threadId];

        uint16_t n = threadsData[REQUESTED_PC_OFFSET][threadId];

        if (n != VM_NO_SETVEC_REQUESTED) {

            threadsData[PC_OFFSET][threadId] = (n == 0xFFFE) ? VM_INACTIVE_THREAD : n;
            threadsData[REQUESTED_PC_OFFSET][threadId] = VM_NO_SETVEC_REQUESTED;
        }
    }
}

void vm_hostFrame(struct VirtualMachine* m) {

    // Run the Virtual Machine for every active threads (one vm frame).
    // Inactive threads are marked with a thread instruction pointer set to 0xFFFF (VM_INACTIVE_THREAD).
    // A thread must feature a break opcode so the interpreter can move to the next thread.

    for (int threadId = 0; threadId < VM_NUM_THREADS; threadId++) {

        if (vmIsChannelActive[CURR_STATE][threadId])
            continue;

        uint16_t n = threadsData[PC_OFFSET][threadId];

        if (n != VM_INACTIVE_THREAD) {

            // Set the script pointer to the right location.
            // script pc is used in executeThread in order
            // to get the next opcode.
            m->_scriptPtr.pc = res->segBytecode + n;
            _stackPtr = 0;

            gotoNextThread = false;
            debug(DBG_VM, "vm_hostFrame() i=0x%02X n=0x%02X *p=0x%02X", threadId, n, *m->_scriptPtr.pc);
            executeThread();

            //Since .pc is going to be modified by this next loop iteration, we need to save it.
            threadsData[PC_OFFSET][threadId] = m->_scriptPtr.pc - res->segBytecode;


            debug(DBG_VM, "vm_hostFrame() i=0x%02X pos=0x%X", threadId, threadsData[PC_OFFSET][threadId]);
            if (sys->input.quit) {
                break;
            }
        }

    }
}

#define COLOR_BLACK 0xFF
#define DEFAULT_ZOOM 0x40


void vm_executeThread(struct VirtualMachine* m) {

    while (!gotoNextThread) {
        uint8_t opcode = m->_scriptPtr.fetchByte();

        // 1000 0000 is set
        if (opcode & 0x80)
        {
            uint16_t off = ((opcode << 8) | m->_scriptPtr.fetchByte()) * 2;
            res->_useSegVideo2 = false;
            int16_t x = m->_scriptPtr.fetchByte();
            int16_t y = m->_scriptPtr.fetchByte();
            int16_t h = y - 199;
            if (h > 0) {
                y = 199;
                x += h;
            }
            debug(DBG_VIDEO, "vid_opcd_0x80 : opcode=0x%X off=0x%X x=%d y=%d", opcode, off, x, y);

            // This switch the polygon database to "cinematic" and probably draws a black polygon
            // over all the screen.
            video->setDataBuffer(res->segCinematic, off);
            video->readAndDrawPolygon(COLOR_BLACK, DEFAULT_ZOOM, Point(x,y));

            continue;
        }

        // 0100 0000 is set
        if (opcode & 0x40)
        {
            int16_t x, y;
            uint16_t off = m->_scriptPtr.fetchWord() * 2;
            x = m->_scriptPtr.fetchByte();

            res->_useSegVideo2 = false;

            if (!(opcode & 0x20))
            {
                if (!(opcode & 0x10))  // 0001 0000 is set
                {
                    x = (x << 8) | m->_scriptPtr.fetchByte();
                } else {
                    x = m->vmVariables[x];
                }
            }
            else
            {
                if (opcode & 0x10) { // 0001 0000 is set
                    x += 0x100;
                }
            }

            y = m->_scriptPtr.fetchByte();

            if (!(opcode & 8))  // 0000 1000 is set
            {
                if (!(opcode & 4)) { // 0000 0100 is set
                    y = (y << 8) | m->_scriptPtr.fetchByte();
                } else {
                    y = m->vmVariables[y];
                }
            }

            uint16_t zoom = m->_scriptPtr.fetchByte();

            if (!(opcode & 2))  // 0000 0010 is set
            {
                if (!(opcode & 1)) // 0000 0001 is set
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

                if (opcode & 1) { // 0000 0001 is set
                    res->_useSegVideo2 = true;
                    --m->_scriptPtr.pc;
                    zoom = 0x40;
                }
            }
            debug(DBG_VIDEO, "vid_opcd_0x40 : off=0x%X x=%d y=%d", off, x, y);
            video->setDataBuffer(res->_useSegVideo2 ? res->_segVideo2 : res->segCinematic, off);
            video->readAndDrawPolygon(0xFF, zoom, Point(x, y));

            continue;
        }


        if (opcode > 0x1A)
        {
            error("vm_executeThread() ec=0x%X invalid opcode=0x%X", 0xFFF, opcode);
        }
        else
        {
            (this->*opcodeTable[opcode])();
        }

    }
}

void vm_inp_updatePlayer(struct VirtualMachine* m) {

    sys->processEvents();

    if (res->currentPartId == 0x3E89) {
        char c = sys->input.lastChar;
        if (c == 8 || /*c == 0xD |*/ c == 0 || (c >= 'a' && c <= 'z')) {
            m->vmVariables[VM_VARIABLE_LAST_KEYCHAR] = c & ~0x20;
            sys->input.lastChar = 0;
        }
    }

    int16_t lr = 0;
    int16_t m = 0;
    int16_t ud = 0;

    if (sys->input.dirMask & PlayerInput::DIR_RIGHT) {
        lr = 1;
        m |= 1;
    }
    if (sys->input.dirMask & PlayerInput::DIR_LEFT) {
        lr = -1;
        m |= 2;
    }
    if (sys->input.dirMask & PlayerInput::DIR_DOWN) {
        ud = 1;
        m |= 4;
    }

    m->vmVariables[VM_VARIABLE_HERO_POS_UP_DOWN] = ud;

    if (sys->input.dirMask & PlayerInput::DIR_UP) {
        m->vmVariables[VM_VARIABLE_HERO_POS_UP_DOWN] = -1;
    }

    if (sys->input.dirMask & PlayerInput::DIR_UP) { // inpJump
        ud = -1;
        m |= 8;
    }

    m->vmVariables[VM_VARIABLE_HERO_POS_JUMP_DOWN] = ud;
    m->vmVariables[VM_VARIABLE_HERO_POS_LEFT_RIGHT] = lr;
    m->vmVariables[VM_VARIABLE_HERO_POS_MASK] = m;
    int16_t button = 0;

    if (sys->input.button) {
        button = 1;
        m |= 0x80;
    }

    m->vmVariables[VM_VARIABLE_HERO_ACTION] = button;
    m->vmVariables[VM_VARIABLE_HERO_ACTION_POS_MASK] = m;
}

void vm_inp_handleSpecialKeys(struct VirtualMachine* m) {

    if (sys->input.pause) {

        if (res->currentPartId != GAME_PART1 && res->currentPartId != GAME_PART2) {
            sys->input.pause = false;
            while (!sys->input.pause) {
                sys->processEvents();
                sys->sleep(200);
            }
        }
        sys->input.pause = false;
    }

    if (sys->input.code) {
        sys->input.code = false;
        if (res->currentPartId != GAME_PART_LAST && res->currentPartId != GAME_PART_FIRST) {
            res->requestedNextPart = GAME_PART_LAST;
        }
    }

    // XXX
    if (m->vmVariables[0xC9] == 1) {
        warning("vm_inp_handleSpecialKeys() unhandled case (m->vmVariables[0xC9] == 1)");
    }

}

void vm_snd_playSound(struct VirtualMachine* m, uint16_t resNum, uint8_t freq, uint8_t vol, uint8_t channel) {

    debug(DBG_SND, "snd_playSound(0x%X, %d, %d, %d)", resNum, freq, vol, channel);

    MemEntry *me = &res->_memList[resNum];

    if (me->state != MEMENTRY_STATE_LOADED)
        return;


    if (vol == 0) {
        mixer->stopChannel(channel);
    } else {
        MixerChunk mc;
        memset(&mc, 0, sizeof(mc));
        mc.data = me->bufPtr + 8; // skip header
        mc.len = READ_BE_UINT16(me->bufPtr) * 2;
        mc.loopLen = READ_BE_UINT16(me->bufPtr + 2) * 2;
        if (mc.loopLen != 0) {
            mc.loopPos = mc.len;
        }
        assert(freq < 40);
        mixer->playChannel(channel & 3, &mc, frequenceTable[freq], MIN(vol, 0x3F));
    }

}

void vm_snd_playMusic(struct VirtualMachine* m, uint16_t resNum, uint16_t delay, uint8_t pos) {

    debug(DBG_SND, "snd_playMusic(0x%X, %d, %d)", resNum, delay, pos);

    if (resNum != 0) {
        player->loadSfxModule(resNum, delay, pos);
        player->start();
    } else if (delay != 0) {
        player->setEventsDelay(delay);
    } else {
        player->stop();
    }
}

void vm_saveOrLoad(struct VirtualMachine* m, Serializer &ser) {
    Serializer::Entry entries[] = {
        SE_ARRAY(m->vmVariables, 0x100, Serializer::SES_INT16, VER(1)),
        SE_ARRAY(_scriptStackCalls, 0x100, Serializer::SES_INT16, VER(1)),
        SE_ARRAY(threadsData, 0x40 * 2, Serializer::SES_INT16, VER(1)),
        SE_ARRAY(vmIsChannelActive, 0x40 * 2, Serializer::SES_INT8, VER(1)),
        SE_END()
    };
    ser.saveOrLoadEntries(entries);
}

void vm_bypassProtection(struct VirtualMachine* m)
{
    File f(true);

    if (!f.open("bank0e", res->getDataDir(), "rb")) {
        warning("Unable to bypass protection: add bank0e file to datadir");
    } else {
        Serializer s(&f, Serializer::SM_LOAD, res->_memPtrStart, 2);
        this->saveOrLoad(s);
        res->saveOrLoad(s);
        video->saveOrLoad(s);
        player->saveOrLoad(s);
        mixer->saveOrLoad(s);
    }
    f.close();
}


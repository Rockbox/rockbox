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

#ifndef __LOGIC_H__
#define __LOGIC_H__



#include "intern.h"

#define VM_NUM_THREADS 64
#define VM_NUM_VARIABLES 256
#define VM_NO_SETVEC_REQUESTED 0xFFFF
#define VM_INACTIVE_THREAD    0xFFFF


#define    VM_VARIABLE_RANDOM_SEED          0x3C

#define    VM_VARIABLE_LAST_KEYCHAR         0xDA

#define    VM_VARIABLE_HERO_POS_UP_DOWN     0xE5
#define    VM_VARIABLE_MUS_MARK             0xF4

#define    VM_VARIABLE_SCROLL_Y             0xF9
#define    VM_VARIABLE_HERO_ACTION          0xFA
#define    VM_VARIABLE_HERO_POS_JUMP_DOWN   0xFB
#define    VM_VARIABLE_HERO_POS_LEFT_RIGHT  0xFC
#define    VM_VARIABLE_HERO_POS_MASK        0xFD
#define    VM_VARIABLE_HERO_ACTION_POS_MASK 0xFE
#define    VM_VARIABLE_PAUSE_SLICES         0xFF

struct Mixer;
struct Resource;
struct Serializer;
struct SfxPlayer;
struct System;
struct Video;

//For threadsData navigation
#define PC_OFFSET 0
#define REQUESTED_PC_OFFSET 1
#define NUM_DATA_FIELDS 2

//For vmIsChannelActive navigation
#define CURR_STATE 0
#define REQUESTED_STATE 1
#define NUM_THREAD_FIELDS 2

struct VirtualMachine;

void vm_create(struct VirtualMachine*, struct Mixer *mix, struct Resource *res, struct SfxPlayer *ply, struct Video *vid, struct System *stub);
void vm_init(struct VirtualMachine*);

void vm_op_movConst(struct VirtualMachine*);
void vm_op_mov(struct VirtualMachine*);
void vm_op_add(struct VirtualMachine*);
void vm_op_addConst(struct VirtualMachine*);
void vm_op_call(struct VirtualMachine*);
void vm_op_ret(struct VirtualMachine*);
void vm_op_pauseThread(struct VirtualMachine*);
void vm_op_jmp(struct VirtualMachine*);
void vm_op_setSetVect(struct VirtualMachine*);
void vm_op_jnz(struct VirtualMachine*);
void vm_op_condJmp(struct VirtualMachine*);
void vm_op_setPalette(struct VirtualMachine*);
void vm_op_resetThread(struct VirtualMachine*);
void vm_op_selectVideoPage(struct VirtualMachine*);
void vm_op_fillVideoPage(struct VirtualMachine*);
void vm_op_copyVideoPage(struct VirtualMachine*);
void vm_op_blitFramebuffer(struct VirtualMachine*);
void vm_op_killThread(struct VirtualMachine*);
void vm_op_drawString(struct VirtualMachine*);
void vm_op_sub(struct VirtualMachine*);
void vm_op_and(struct VirtualMachine*);
void vm_op_or(struct VirtualMachine*);
void vm_op_shl(struct VirtualMachine*);
void vm_op_shr(struct VirtualMachine*);
void vm_op_playSound(struct VirtualMachine*);
void vm_op_updateMemList(struct VirtualMachine*);
void vm_op_playMusic(struct VirtualMachine*);

void vm_initForPart(struct VirtualMachine*, uint16_t partId);
void vm_setupPart(struct VirtualMachine*, uint16_t partId);
void vm_checkThreadRequests(struct VirtualMachine*);
void vm_hostFrame(struct VirtualMachine*);
void vm_executeThread(struct VirtualMachine*);

void vm_inp_updatePlayer(struct VirtualMachine*);
void vm_inp_handleSpecialKeys(struct VirtualMachine*);

void vm_snd_playSound(struct VirtualMachine*, uint16_t resNum, uint8_t freq, uint8_t vol, uint8_t channel);
void vm_snd_playMusic(struct VirtualMachine*, uint16_t resNum, uint16_t delay, uint8_t pos);

void vm_saveOrLoad(struct VirtualMachine*, struct Serializer *ser);
void vm_bypassProtection(struct VirtualMachine*);

typedef void (*OpcodeStub)(struct VirtualMachine*);

// The type of entries in opcodeTable. This allows "fast" branching
static const OpcodeStub vm_opcodeTable[] = {
    /* 0x00 */
    &vm_op_movConst,
    &vm_op_mov,
    &vm_op_add,
    &vm_op_addConst,
    /* 0x04 */
    &vm_op_call,
    &vm_op_ret,
    &vm_op_pauseThread,
    &vm_op_jmp,
    /* 0x08 */
    &vm_op_setSetVect,
    &vm_op_jnz,
    &vm_op_condJmp,
    &vm_op_setPalette,
    /* 0x0C */
    &vm_op_resetThread,
    &vm_op_selectVideoPage,
    &vm_op_fillVideoPage,
    &vm_op_copyVideoPage,
    /* 0x10 */
    &vm_op_blitFramebuffer,
    &vm_op_killThread,
    &vm_op_drawString,
    &vm_op_sub,
    /* 0x14 */
    &vm_op_and,
    &vm_op_or,
    &vm_op_shl,
    &vm_op_shr,
    /* 0x18 */
    &vm_op_playSound,
    &vm_op_updateMemList,
    &vm_op_playMusic
};

struct VirtualMachine {
    //This table is used to play a sound
    //static const uint16_t frequenceTable[];
    /* FW: moved from staticres.c to vm.c */

    struct Mixer *mixer;
    struct Resource *res;
    struct SfxPlayer *player;
    struct Video *video;
    struct System *sys;

    int16_t vmVariables[VM_NUM_VARIABLES];
    uint16_t _scriptStackCalls[VM_NUM_THREADS];

    uint16_t threadsData[NUM_DATA_FIELDS][VM_NUM_THREADS];
    // This array is used:
    //     0 to save the channel's instruction pointer
    //     when the channel release control (this happens on a break).

    //     1 When a setVec is requested for the next vm frame.

    uint8_t vmIsChannelActive[NUM_THREAD_FIELDS][VM_NUM_THREADS];

    struct Ptr _scriptPtr;
    uint8_t _stackPtr;
    bool gotoNextThread;
    bool _fastMode;
};
#endif

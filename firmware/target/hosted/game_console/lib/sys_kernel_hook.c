/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2026 Mauricio Garrido
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

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "debug.h"
#include "logf.h"

#include "sys_kernel_hook.h"

#include <psptypes.h>
#include <systemctrl.h>
/* todo: conflict declaration in */
/* #include <kubridge.h> */

struct KernelCallArg {
    u32 arg1;
    u32 arg2;
    u32 arg3;
    u32 arg4;
    u32 arg5;
    u32 arg6;
    u32 arg7;
    u32 arg8;
    u32 arg9;
    u32 arg10;
    u32 arg11;
    u32 arg12;
    u32 ret1;
    u32 ret2;
};

int kuKernelCall(void *func_addr, struct KernelCallArg *args);

static void *_sceDisplayEnable = NULL; // int (*)(void)
static void *_sceDisplayDisable = NULL; // int (*)(void)
static void *_sceDisplayGetBrightness = NULL; // void (*)(int *, int *)
static void *_sceDisplaySetBrightness = NULL; // void (*)(int, int)

static void *_sceImposeGetParam = NULL; // int (*)(int)
static void *_sceImposeSetParam = NULL; // int (*)(int, int)

static void *_sceAudioSetFrequency = NULL; // int (*)(int)

static void *_sceCtrlPeekBufferPositive = NULL; // int (*)(SceCtrlData *, int)

void sys_init_kernel_hook(void)
{
    // get CFW version
    u32 ver = sctrlHENGetVersion();

    // get OFW version (bypass patches)
    struct KernelCallArg args;
    u32 getDevkitVersion = sctrlHENFindFunction("sceSystemMemoryManager", "SysMemUserForUser", 0x3FC9AE6A);
    kuKernelCall((void*)getDevkitVersion, &args);

    u32 fw = args.ret1;
    logf("running on CFW: 0x%08x\n", ver);
    logf("OFW version: 0x%08x\n", fw);

    _sceImposeGetParam = (void *)sctrlHENFindFunction("sceImpose_Driver", "sceImpose_driver", 0x531C9778);
    _sceImposeSetParam = (void *)sctrlHENFindFunction("sceImpose_Driver", "sceImpose_driver", 0x810FB7FB);

    _sceDisplayEnable = (void *)sctrlHENFindFunction("sceDisplay_Service", "sceDisplay_driver", 0x432D133F);
    _sceDisplayDisable = (void *)sctrlHENFindFunction("sceDisplay_Service", "sceDisplay_driver", 0x681EE6A7);
    _sceDisplayGetBrightness = (void *)sctrlHENFindFunction("sceDisplay_Service", "sceDisplay_driver", 0x31C4BAA8);
    _sceDisplaySetBrightness = (void *)sctrlHENFindFunction("sceDisplay_Service", "sceDisplay_driver", 0x9E3C6DC6);

    _sceAudioSetFrequency = (void *)sctrlHENFindFunction("sceAudio_Driver", "sceAudio_driver", 0xA2BEAA6C);

    _sceCtrlPeekBufferPositive = (void *)sctrlHENFindFunction("sceCtrl_Driver", "sceCtrl_driver", 0x3A622550);
    if (_sceCtrlPeekBufferPositive == NULL) {
        logf("sctrlHENFindFunction: failed to find function 0x3A622550\n");
    }
}

int sys_display_enable(void)
{
    if (_sceDisplayEnable) {
        struct KernelCallArg args;
        kuKernelCall(_sceDisplayEnable, &args);
        return args.ret1;
    }

    return -1;
}

int sys_display_disable(void)
{
    if (_sceDisplayDisable) {
        struct KernelCallArg args;
        kuKernelCall(_sceDisplayEnable, &args);
        return args.ret1;
    }

    return -1;
}

void sys_display_get_brightness(int *level)
{
    if (_sceDisplayGetBrightness) {
        struct KernelCallArg args;
        args.arg1 = (uint32_t) level;
        args.arg2 = 0;
        kuKernelCall(_sceDisplayGetBrightness, &args);
    }
}

void sys_display_set_brightness(int level)
{
    if (_sceDisplaySetBrightness) {
        struct KernelCallArg args;
        args.arg1 = level;
        args.arg2 = 0;
        kuKernelCall(_sceDisplaySetBrightness, &args);
    }
}

int sys_impose_get_param(SceImposeParam param)
{
    if (_sceImposeGetParam) {
        struct KernelCallArg args;
        args.arg1 = param;
        kuKernelCall(_sceImposeGetParam, &args);
        return args.ret1;
    }

    return -1;
}

int sys_impose_set_param(SceImposeParam param, int value)
{
    if (_sceImposeSetParam) {
        struct KernelCallArg args;
        args.arg1 = param;
        args.arg2 = value;
        kuKernelCall(_sceImposeSetParam, &args);
        return args.ret1;
    }

    return -1;
}

int sys_audio_set_frequency(int frequency)
{
    if (_sceAudioSetFrequency) {
        struct KernelCallArg args;
        args.arg1 = frequency;
        kuKernelCall(_sceAudioSetFrequency, &args);
        return args.ret1;
    }

    return -1;
}

int sys_ctrl_peek_buffer_positive(SceCtrlData *pad_data, int count)
{
    if (_sceCtrlPeekBufferPositive) {
        struct KernelCallArg args;
        args.arg1 = (uint32_t) pad_data;
        args.arg2 = count;
        kuKernelCall(_sceCtrlPeekBufferPositive, &args);
        return args.ret1;
    }

    return -1;
}


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

#ifndef __XWORLD_SYS_H__
#define __XWORLD_SYS_H__

#include "plugin.h"
#include "intern.h"

#if (PLUGIN_BUFFER_SIZE >= 0x80000 && defined(HAVE_LCD_COLOR) && LCD_DEPTH < 24)
#define SYS_MOTION_BLUR
/* must be odd */
#define BLUR_FRAMES 3
#endif

#define NUM_COLORS 16
#define MAX_MUTEXES 16
#define SETTINGS_FILE "settings.zfg" /* change when backwards-compatibility is broken */
#define CODE_X 80
#define CODE_Y 36

enum {
    DIR_LEFT  = 1 << 0,
    DIR_RIGHT = 1 << 1,
    DIR_UP    = 1 << 2,
    DIR_DOWN  = 1 << 3
};

struct PlayerInput {

    uint8_t dirMask;
    bool button;
    bool code;
    bool pause;
    bool quit;
    char lastChar;
    bool save, load;
    bool fastMode;
    int8_t stateSlot;
};


struct keymapping_t {
    int up;
    int down;
    int left;
    int right;
#if (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
    int upleft;
    int upright;
    int downleft;
    int downright;
#endif
};

typedef void (*AudioCallback)(void *param, uint8_t *stream, int len);
typedef uint32_t (*TimerCallback)(uint32_t delay, void *param);

struct System {
    struct mutex mutex_memory[MAX_MUTEXES];
    uint16_t mutex_bitfield;
    struct PlayerInput input;
    fb_data palette[NUM_COLORS];
    struct Engine* e;
    struct keymapping_t keymap;

    void *membuf;
    size_t bytes_left;

    bool loaded;

    struct {
        bool negative_enabled;
        /*
          scaling quality:
          0: off
          1: fast
          2: good (color only)
        */
        int scaling_quality;
        /*
          rotation:
          0: off
          1: cw
          2: ccw
        */
        int rotation_option;

        bool showfps;
        bool sound_enabled;
        int sound_bufsize;
        bool zoom;
        bool blur;
    } settings;
};

void sys_init(struct System*, const char *title);
void sys_menu(struct System*);
void sys_destroy(struct System*);

void sys_setPalette(struct System*, uint8_t s, uint8_t n, const uint8_t *buf);
void sys_copyRect(struct System*, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *buf, uint32_t pitch);

void sys_processEvents(struct System*);
void sys_sleep(struct System*, uint32_t duration);
uint32_t sys_getTimeStamp(struct System*);

void sys_startAudio(struct System*, AudioCallback callback, void *param);
void sys_stopAudio(struct System*);
uint32_t sys_getOutputSampleRate(struct System*);

void *sys_addTimer(struct System*, uint32_t delay, TimerCallback callback, void *param);
void sys_removeTimer(struct System*, void *timerId);

void *sys_createMutex(struct System*);
void sys_destroyMutex(struct System*, void *mutex);
void sys_lockMutex(struct System*, void *mutex);
void sys_unlockMutex(struct System*, void *mutex);

/* a quick 'n dirty function to get some bytes in the audio buffer after zeroing them */
/* pretty much does the same thing as calloc, though much uglier and there's no free() */
void *sys_get_buffer(struct System*, size_t);

uint8_t* getOffScreenFramebuffer(struct System*);

struct MutexStack_t {
    struct System *sys;
    void *_mutex;
};

void MutexStack(struct MutexStack_t*, struct System*, void*);
void MutexStack_destroy(struct MutexStack_t*);

#endif

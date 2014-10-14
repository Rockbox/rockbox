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

#ifndef __XWORLD_SYS_H__
#define __XWORLD_SYS_H__

#include "intern.h"

#define SYS_ANTIALIASING
#define SYS_MOTION_BLUR
#define NUM_BLUR_FRAMES 16
#define SYS_NEGATIVE_COLOR
#define TITLE_SHOW_TIME (10*HZ)
#define TITLE_TEXT "Press any key..."
#define TITLE_ANIM_STEPS 64
#define TITLE_ANIM_SLEEP (HZ/20)
#define TITLE_ANIM_BLUR_LEVEL 1
#define TITLE_ANIM_DIVISOR 6 /* preferably a power of two */
#define NUM_COLORS 16
#define BYTE_PER_PIXEL 3
#define MAX_MUTEXES 16

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

/*
  System is an abstract class so any find of system can be plugged underneath.
*/
typedef void (*AudioCallback)(void *param, uint8_t *stream, int len);
typedef uint32_t (*TimerCallback)(uint32_t delay, void *param);
struct System {
    struct mutex mutex_memory[MAX_MUTEXES];
    uint16_t mutex_bitfield;
    struct PlayerInput input;
    fb_data pallete[NUM_COLORS];
    struct Engine* e;
    struct keymapping_t keymap;
    bool motion_blur_enabled;
    bool antialias_enabled;
    bool negative_enabled;
/*
  scaling:
  0: off
  1: fast
  2: good (color only)
*/
    int scaling_option;
/*
  rotation:
  0: off
  1: cw
  2: ccw
*/
    int rotation_option;

    bool loaded;
<<<<<<< HEAD
=======
    bool showfps;
>>>>>>> 63f4a8f... [WIP] Xworld Another World interpreter for Rockbox
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

uint8_t* getOffScreenFramebuffer(struct System*);

struct MutexStack_t {
    struct System *sys;
    void *_mutex;
};

void MutexStack(struct MutexStack_t*, struct System*, void*);
void MutexStack_destroy(struct MutexStack_t*);

#endif

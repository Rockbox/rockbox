#ifndef SOUND_H_
#define SOUND_H_

#include "Directories.h"
#include "Main.h"

#define DOOR_SOUND 0
#define OBJECT_SOUND 1
#define POP_SOUND 2
#define CLOSED_SOUND 3
#define COIL_SOUND 4
#define BUBBLE_SOUND 5
#define MENU_SOUND 6
#define DEATH_SOUND 7
#define PLATFORM_SOUND 8
#define JUMP_SOUND 9
#define GEM_SOUND 10

extern int sound_enabled, music_enabled;
extern int sound_loaded;

void initAudio();
void playSound(int index);
void playIntroMusic();
void playGameMusic();
void stopMusic();

#endif

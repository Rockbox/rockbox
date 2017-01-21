#ifndef MAIN_H_
#define MAIN_H_

#include "SDL.h"

#include "Common.h"
#include "Image.h"
#include "Font.h"
#include "Util.h"
#include "Sound.h"
#include "Splash.h"
#include "Menu.h"
#include "Map.h"
#include "MapIO.h"
#include "Monster.h"
#include "Editor.h"
#include "Game.h"

#define RUNMODE_SPLASH 0
#define RUNMODE_EDITOR 1
#define RUNMODE_GAME 2

extern int runmode;

extern SDL_Surface *screen;
extern int state;

typedef struct _main {
  int drawBackground;
  int full_screen;
  int alphaBlend;
  int effects_enabled;
} Main;
extern Main mainstruct;

void startEditor();
void startGame();
void showLoadingProgress();

#endif

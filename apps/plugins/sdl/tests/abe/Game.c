#include "Game.h"
#include <errno.h>
#include <string.h>

Game game;

// path_sprintf should not be used by other .c files, as it does not fit for them.
static void
path_sprintf(char *path, char *formatted_name, int version)
{

  int len;

  printf("path_sprintf (%p, %s, %d)\n", path, formatted_name, version);

  rb->strlcpy(path, "/.abe/", 999);

  len = rb->strlen(path);

  if(1 == version) {
      rb->snprintf(path + len, 999, formatted_name);
  } else {
      rb->snprintf(path + len, 999, formatted_name, version);
  }

}

void
deleteSavedGame()
{
  char path[PATH_SIZE];
  // version 2
  path_sprintf(path, "save%d.dat", GAME_VERSION);
  rb->remove(path);
  path_sprintf(path, "savedmap%d.dat", GAME_VERSION);
  rb->remove(path);
  // version 1
  path_sprintf(path, "save.dat", 1);
  rb->remove(path);
  path_sprintf(path, "savedmap.dat", 1);
  rb->remove(path);
}

void
saveGame()
{
  char path[PATH_SIZE];
  FILE *fp;
  char *err;
  SDL_RWops *rwop;

  mkshuae();

  path_sprintf(path, "save%d.dat", GAME_VERSION);

  if(!(fp = fopen(path, "wb"))) {
    return;
  }
  rwop = SDL_RWFromFP(fp, 1);

  // FIXME: should also write map creation date so old savegame 
  // won't be loaded over newer map. (versioning)

  // write game state
  SDL_WriteLE16(rwop, cursor.pos_x);
  SDL_WriteLE16(rwop, cursor.pos_y);
  SDL_WriteLE16(rwop, cursor.pixel_x);
  SDL_WriteLE16(rwop, cursor.pixel_y);
  SDL_WriteLE16(rwop, game.lives);
  SDL_WriteLE16(rwop, game.score);
  SDL_WriteLE16(rwop, game.keys);
  SDL_WriteLE16(rwop, game.balloons);
  SDL_WriteLE16(rwop, game.lastSavePosX);
  SDL_WriteLE16(rwop, game.lastSavePosY);

  SDL_RWclose(rwop);

  // save the map
  path_sprintf(path, "savedmap%d.dat", GAME_VERSION);
  saveMapPath(path);
}

// returns 0 if no game saved, 1 otherwise
int
loadGame()
{
  char path[PATH_SIZE];
  FILE *fp;
  //char *err;
  SDL_RWops *rwop;
  int version;

  version = (int) GAME_VERSION;

  // load the map
  path_sprintf(path, "savedmap%d.dat", GAME_VERSION);
  if(!loadMapPath(path, 0)) {
    // if can't find saved map load static map
    fprintf(stderr,
            "Can't find current saved map. Will try to use static map.\n");
    if(!loadMap(0)) {
      fprintf(stderr, "Can't find map file: %s\n", map.name);
      fflush(stderr);
      return 0;
    }
  }
  // we have to do this _after_ loading the map
  // b/c it resets the cursor
  // try to find a saved game of any version
  while(version > 0) {
    if(version > 1) {
      path_sprintf(path, "save%d.dat", version);
    } else {                    // By Pedro: version==1
      path_sprintf(path, "save.dat", version);
    }
    fprintf(stderr, "Trying to load saved game: %s\n", path);
    fflush(stderr);
    fp = fopen(path, "rb");
    if(fp) {
      fprintf(stderr, "Saved game found. %s\n", path);
      fflush(stderr);
      break;
    }
    fprintf(stderr, "Saved game not found. %s\n", path);
    fflush(stderr);
    version--;
  }
  if(version == 0) {
    fprintf(stderr, "Can't find or open saved game.\n");
    fflush(stderr);
    return 0;
  }
  rwop = SDL_RWFromFP(fp, 1);

  // read game state
  cursor.pos_x = SDL_ReadLE16(rwop);
  cursor.pos_y = SDL_ReadLE16(rwop);
  cursor.pixel_x = SDL_ReadLE16(rwop);
  cursor.pixel_y = SDL_ReadLE16(rwop);
  game.lives = SDL_ReadLE16(rwop);
  game.score = SDL_ReadLE16(rwop);
  game.keys = SDL_ReadLE16(rwop);
  game.balloons = SDL_ReadLE16(rwop);
  game.lastSavePosX = SDL_ReadLE16(rwop);
  game.lastSavePosY = SDL_ReadLE16(rwop);

  game.player_start_x = cursor.pos_x;
  game.player_start_y = cursor.pos_y;

  SDL_RWclose(rwop);

  return 1;
}

int
getGameFace()
{
  // change the face
  if(cursor.jump || cursor.slide) {
    game.face = (game.dir == DIR_LEFT ? 8 : 9);
    return game.face;
  }
  if(game.balloonTimer) {
    game.face = (game.dir == DIR_LEFT ? 6 : 7);
    game.balloonTimer--;
    if(game.balloonTimer <= 0) {
      game.balloonTimer = 0;
      map.gravity = 1;
      playSound(POP_SOUND);
    } else {
      return game.face;
    }
  }
  if(interact.on_ladder) {
    if(cursor.dir & DIR_LEFT || cursor.dir & DIR_RIGHT ||
       cursor.dir & DIR_UP || cursor.dir & DIR_DOWN) {
      game.face++;
    }
    if(game.face >= CLIMB_FACE_COUNT * FACE_STEP)
      game.face = 0;
    return (game.face / FACE_STEP + CLIMB_FACE_MIN);
  }
  if(game.dir_changed) {
    game.face++;
    if(game.face >= FACE_STEP) {
      game.dir_changed = 0;
    }
    return 11;
  }
  if(cursor.dir & DIR_LEFT || cursor.dir & DIR_RIGHT) {
    game.face++;
  }
  if(game.face >= FACE_COUNT * FACE_STEP)
    game.face = 0;
  return (game.dir == DIR_LEFT ?
          (game.face / FACE_STEP) : (game.face / FACE_STEP) + FACE_COUNT);
}

/**
   The editor-specific map drawing event.
   This is called before the map is sent to the screen.
 */
void
afterMainLevelDrawn()
{
  SDL_Rect pos;

  // draw Tom
  int screen_center_x = (screen->w / TILE_W) / 2;
  int screen_center_y = (screen->h / TILE_H) / 2;
  pos.x = screen_center_x * TILE_W;
  pos.y = screen_center_y * TILE_H;
  pos.w = tom[0]->w;
  pos.h = tom[0]->h;

  if(game.draw_player) {
    SDL_BlitSurface(tom[getGameFace()], NULL, screen, &pos);
  }

  if(mainstruct.effects_enabled) {
    processEffects();
  }
}

void
gameBeforeDrawToScreen()
{
  SDL_Rect rect;
  char s[80];
  int x, y, w, h;
  double u;
  Uint32 color;

  // draw score board
  SDL_BlitSurface(score_image, NULL, screen, NULL);
  rb->snprintf(s, 999, "%d", game.keys);
  drawString(screen, 132, 10, s);
  rb->snprintf(s, 999, "%d", game.balloons);
  drawString(screen, 190, 10, s);
  rb->snprintf(s, 999, "%d", game.lives);
  drawString(screen, 257, 10, s);
  //  drawString(screen, 190, 39, s);
  rb->snprintf(s, 999, "%d", game.score);
  drawString(screen, 190, 67, s);

  // draw the health-bar
  x = 187;
  y = 38;
  // 0.87 = max_width / max_health = 87 / 100
  w = (int) ((double) game.health * 0.87);
  h = 22;
  rect.x = x;
  rect.y = y;
  rect.w = w;
  rect.h = h;
  if(game.in_water) {
    color = SDL_MapRGBA(screen->format, 0x00, 0x60, 0xe0, 0x00);
  } else {
    color =
      SDL_MapRGBA(screen->format, 0xe0, (game.health > 30 ? 0xa0 : 0x00),
                  0x00, 0x00);
  }
  SDL_FillRect(screen, &rect, color);


  if(game.end_game) {
    drawString(screen, 50, 10 + FONT_HEIGHT * 6, "success!!!");
    drawString(screen, 50, 10 + FONT_HEIGHT * 7, "abe rescued his friend");
    drawString(screen, 50, 10 + FONT_HEIGHT * 8,
               "from the cavernous prison of");
    drawString(screen, 50, 10 + FONT_HEIGHT * 9, "the great pyramid!");
    drawString(screen, screen->w / 2 - 100, screen->h - 30, "press escape");
  }
#if GOD_MODE
  drawString(screen, 255, 67, (game.god_mode ? "t" : "f"));

  rb->snprintf(s, 999, "%s%s%s%s",
               (cursor.dir & DIR_UP ? "u" : " "),
               (cursor.dir & DIR_DOWN ? "d" : " "),
               (cursor.dir & DIR_LEFT ? "l" : " "),
               (cursor.dir & DIR_RIGHT ? "r" : " "));
  drawString(screen, 0, screen->h - 40, s);
  rb->snprintf(s, 999, "d %d f %d t %d b %d h2o %d", map.delta, map.fps_override,
          (1000 / FPS_THROTTLE), map.max_speed_boost, game.in_water);
  drawString(screen, 0, screen->h - 20, s);
  /* sprintf(s, "pos %d %d", cursor.pos_x, cursor.pos_y);
     drawString(screen, 0, screen->h - 60, s);
     sprintf(s, "pixel %d %d", cursor.pixel_x, cursor.pixel_y);
     drawString(screen, 0, screen->h - 40, s);
     sprintf(s, "speed %d %d", cursor.speed_x, cursor.speed_y);
     drawString(screen, 0, screen->h - 20, s);
   */
#endif

  // draw the balloon timer
  if(game.balloonTimer > 0) {
    x = 5;
    y = 94;
    u = (double) 269 / (double) BALLOON_RIDE_INTERVAL;
    w = (int) ((double) game.balloonTimer * u);
    h = 2;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    SDL_FillRect(screen, &rect,
                 SDL_MapRGBA(screen->format, 0xe0, 0xa0, 0x00, 0x00));
  }
}

void
setGameDir()
{
  if(cursor.dir & DIR_LEFT) {
    if(game.dir != DIR_LEFT) {
      game.dir_changed = 1;
      game.face = 0;
    }
    game.dir = DIR_LEFT;
  } else if(cursor.dir & DIR_RIGHT) {
    if(game.dir != DIR_RIGHT) {
      game.dir_changed = 1;
      game.face = 0;
    }
    game.dir = DIR_RIGHT;
  }
}

/**
   Main game event handling
*/
void
gameMainLoop(SDL_Event * event)
{
    if(GOD_MODE)
        game.god_mode = 1;
  if(!move_monsters)
    return;

  switch (event->type) {
  case SDL_KEYDOWN:
    // printf("The %s key was pressed! scan=%d\n", SDL_GetKeyName(event->key.keysym.sym), event->key.keysym.scancode);
    switch (event->key.keysym.sym) {
    case SDLK_LEFT:
      if(!game.end_game) {
        cursor.dir |= DIR_LEFT;
        cursor.speed_x = START_SPEED_X;
      }
      break;
    case SDLK_RIGHT:
      if(!game.end_game) {
        cursor.dir |= DIR_RIGHT;
        cursor.speed_x = START_SPEED_X;
      }
      break;
    case SDLK_UP:
      if(!game.end_game) {
        cursor.dir |= DIR_UP;
        cursor.speed_y = START_SPEED_Y;
      }
      break;
    case SDLK_DOWN:
      if(!game.end_game) {
        cursor.dir |= DIR_DOWN;
        cursor.speed_y = START_SPEED_Y;
      }
      break;
    case SDLK_r:
      drawMap();
      break;
    case SDLK_SPACE:
      if(!game.end_game) {
        if(startJump())
          playSound(JUMP_SOUND);
      }
      break;
    case SDLK_ESCAPE:
      map.quit = 1;
      break;
    case SDLK_d:
      debugMonsters();
      break;
#if GOD_MODE
    case SDLK_k:
      game.keys++;
      break;
#endif
    case SDLK_RETURN:
      if(!game.end_game) {
        if(!game.balloonTimer && game.balloons) {
          playSound(BUBBLE_SOUND);
          game.balloons--;
          game.balloonTimer = BALLOON_RIDE_INTERVAL;
          map.gravity = 0;
        }
      }
      break;
    case SDLK_s:
      drawMap();
      SDL_SaveBMP(screen, "screenshot.bmp");
      fprintf(stderr, "Saved screenshot.bmp.\n");
      fflush(stderr);
      break;
    case SDLK_g:
      if(GOD_MODE) {
        game.god_mode = !(game.god_mode);
        drawMap();
      }
      break;
    // Pedro: ALT+TAB minimize.
    case SDLK_TAB:
      if((KMOD_LALT==event->key.keysym.mod)||(KMOD_RALT==event->key.keysym.mod)) {
        SDL_WM_IconifyWindow();
      }
    break;
    default:
      break;
    }
    break;
  case SDL_KEYUP:
    switch (event->key.keysym.sym) {
    case SDLK_LEFT:
      cursor.dir &= 15 - DIR_LEFT;
      //   if(cursor.dir == DIR_LEFT) cursor.dir = DIR_UPDATE;
      break;
    case SDLK_RIGHT:
      cursor.dir &= 15 - DIR_RIGHT;
      //   if(cursor.dir == DIR_RIGHT) cursor.dir = DIR_UPDATE;
      break;
    case SDLK_UP:
      cursor.dir &= 15 - DIR_UP;
      //   if(cursor.dir == DIR_UP) cursor.dir = DIR_UPDATE;
      break;
    case SDLK_DOWN:
      cursor.dir &= 15 - DIR_DOWN;
      //   if(cursor.dir == DIR_DOWN) cursor.dir = DIR_UPDATE;
      break;
    default:
      break;
    }
    break;
  }

  setGameDir();
}

void
handleDeath(LiveMonster * live)
{
  int i;
  SDL_Rect fx;
  int screen_center_x = (screen->w / TILE_W) / 2;
  int screen_center_y = (screen->h / TILE_H) / 2;
  int show_effect = 1;

  if(live) {
    showMapStatus(live->monster->name);
    game.health -= (live->monster->damage * (game.difficoulty + 1));
  } else {
    showMapStatus("drowning!");
    game.health--;              // water damage
    show_effect = !(game.tick % 40);
  }
  if(show_effect && mainstruct.effects_enabled) {
    fx.x = screen_center_x * TILE_W;
    fx.y = screen_center_y * TILE_H;
    fx.w = tom[0]->w;
    fx.h = tom[0]->h;
    damageEffect(&fx, screen);
  }


  if(game.health > 0)
    return;
  game.health = MAX_HEALTH;

  playSound(DEATH_SOUND);
  if(live) {
    fprintf(stderr,
            "Player death! Killed by %s at x=%d y=%d pixelx=%d pixely=%d\n",
            live->monster->name, cursor.pos_x, cursor.pos_y, cursor.pixel_x,
            cursor.pixel_y);
  } else {
    fprintf(stderr, "Player drowned.\n");
  }
  fflush(stderr);

  if(game.god_mode)
    return;

  game.lives--;

  // Flash player. Don't move monsters during this.
  move_monsters = 0;
  for(i = 0; i < 5; i++) {
    game.draw_player = 0;
    drawMap();
    SDL_Delay(200);
    game.draw_player = 1;
    drawMap();
    SDL_Delay(200);
  }
  move_monsters = 1;

  if(game.lives <= 0) {
    deleteSavedGame();
    map.quit = 1;
    return;
  }

  repositionCursor(game.player_start_x, game.player_start_y);
  cursor.speed_x = 0;
  cursor.speed_y = 0;
  drawMap();
}

/**
   This is called with the cursor position established.
   Do checks here that need precision, like monster hits,
   platform jumps, etc.
 */
void
gameCheckPosition()
{
  int n;
  Position pos, pos2, pos3, key;
  LiveMonster *live;
  SDL_Rect fx;
  int ignore = 0;
  int screen_center_x = (screen->w / TILE_W) / 2;
  int screen_center_y = (screen->h / TILE_H) / 2;

  pos.pos_x = cursor.pos_x;
  pos.pos_y = cursor.pos_y;
  pos.pixel_x = cursor.pixel_x;
  pos.pixel_y = cursor.pixel_y;
  pos.w = tom[0]->w / TILE_W;
  pos.h = tom[0]->h / TILE_H;

  live = detectMonster(&pos);

  // did we hit a monster?
  if(live) {
    if(!live->monster->harmless) {
      handleDeath(live);
    } else if(live->monster->type == MONSTER_END_GAME) {
      // end of game?
      *((int *) (live->custom)) = 1;
      game.end_game = 1;
    } else if(live->monster->type == MONSTER_STAR) {
      if(game.lastSavePosX != live->pos_x && game.lastSavePosX != live->pos_y) {
        game.lastSavePosX = live->pos_x;
        game.lastSavePosX = live->pos_y;
        game.player_start_x = cursor.pos_x;
        game.player_start_y = cursor.pos_y;
        removeAllLiveMonsters();
        saveGame();
        drawString(screen, 150, 220, "game saved!");
        SDL_Flip(screen);
        SDL_Delay(3000);
        map.redraw = 1;
      }
    }
  }
  // did we hit a platform?
  pos2.pos_x = cursor.pos_x;
  pos2.pos_y = cursor.pos_y + (tom[0]->h / TILE_H) - 1;
  pos2.pixel_x = cursor.pixel_x;
  pos2.pixel_y = cursor.pixel_y;
  pos2.w = tom[0]->w / TILE_W;
  pos2.h = 2;
  live = detectMonster(&pos2);
  if(live &&
     (live->monster->type == MONSTER_PLATFORM
      || live->monster->type == MONSTER_PLATFORM2) && !game.balloonTimer) {
    if(!cursor.platform)
      playSound(PLATFORM_SOUND);
    cursor.platform = live;
  } else {
    cursor.platform = NULL;
  }

  // did we hit an object? 
  // FIXME: this fails if there many objects close together.
  // maybe it should return a NULL terminated array of positions.
  if(containsTypeWhere(&pos, &key, TYPE_OBJECT)) {
    n = map.image_index[LEVEL_MAIN][key.pos_x + (key.pos_y * map.w)];
    if(n == img_key) {
      if(game.keys < MAX_KEYS)
        game.keys++;
      else
        ignore = 1;
    } else if(n == img_balloon[0] || n == img_balloon[1]
              || n == img_balloon[2]) {
      if(game.balloons < MAX_BALLOONS)
        game.balloons++;
      else
        ignore = 1;
    } else if(n == img_gem[0]) {
      game.score++;
    } else if(n == img_gem[1]) {
      game.score += 5;
    } else if(n == img_gem[2]) {
      game.score += 10;
    } else if(n == img_health) {
      if(game.health >= MAX_HEALTH - 10)
        ignore = 1;
      else
        game.health += 10;
    }
    if(!ignore) {
      if(mainstruct.effects_enabled) {
        fx.x = screen_center_x * TILE_W;
        fx.y = screen_center_y * TILE_H;
        fx.w = tom[0]->w;
        fx.h = tom[0]->h;
        shimmerEffect(&fx, screen);
      }
      playSound(n == img_gem[0] || n == img_gem[1]
                || n == img_gem[2] ? GEM_SOUND : OBJECT_SOUND);
      // remove from map
      map.image_index[LEVEL_MAIN][key.pos_x + (key.pos_y * map.w)] =
        EMPTY_MAP;
      map.redraw = 1;
    }
  }
  // in water?
  pos3.pos_x = cursor.pos_x;
  pos3.pos_y = cursor.pos_y;
  pos3.pixel_x = cursor.pixel_x;
  pos3.pixel_y = cursor.pixel_y;
  pos3.w = tom[0]->w / TILE_W;
  pos3.h = 1;
  game.in_water = containsTypeInLevel(&pos3, &key, TYPE_HARMFUL, LEVEL_FORE);
  if(game.in_water && !(game.tick % 10) && game.health > 0)
    handleDeath(NULL);

  // did we hit a spring
  if(containsType(&pos, TYPE_SPRING)) {
    playSound(COIL_SOUND);
    startJumpN(SPRING_JUMP);
  }
  // healing
  if(game.health < MAX_HEALTH) {
    switch (game.difficoulty) {
    case 0:
      if(!(game.tick % 50))
        game.health++;
      break;
    case 1:
      if(!(game.tick % 100))
        game.health++;
      break;
    default:
      break;
    }
  }
  if(game.difficoulty < 2 && !(game.tick % 100) && game.health < MAX_HEALTH)
    game.health++;

  // advance the timer
  game.tick++;
  if(game.tick == 10000)
    game.tick = 0;
}

/**
   This is called with the cursor position proposed.
   (that is the player could fail to move there.)
   return a 1 to proceed, 0 to stop
 */
int
detectCollision(int dir)
{
  Position pos, key;

  pos.pos_x = cursor.pos_x;
  pos.pos_y = cursor.pos_y;
  pos.pixel_x = cursor.pixel_x;
  pos.pixel_y = cursor.pixel_y;
  pos.w = tom[0]->w / TILE_W;
  pos.h = tom[0]->h / TILE_H;

  // did we hit a door?
  if(containsTypeWhere(&pos, &key, TYPE_DOOR)) {
    if(game.keys > 0) {
      // open door
      playSound(DOOR_SOUND);
      map.image_index[LEVEL_MAIN][key.pos_x + (key.pos_y * map.w)] =
        EMPTY_MAP;
      map.image_index[LEVEL_FORE][key.pos_x + (key.pos_y * map.w)] =
        img_door2;
      game.keys--;
      map.redraw = 1;
      // always return 0 (block) so we don't fall into a door and get stuck there... (was a nasty bug)
      return 0;
    } else {
      playSound(CLOSED_SOUND);
      return 0;
    }
  }
  // are we in a wall?
  return !containsType(&pos, TYPE_WALL);
}

// return: 0 - no ladder, 1 - on ladder, can only move down, 2 - on ladder can move up or down
int
detectLadder()
{
  Position pos, where;
  int ret;

  if(game.balloonTimer)
    return 2;                   // With a balloon you can move where you want

  pos.pos_x = cursor.pos_x;
  pos.pos_y = cursor.pos_y;
  pos.pixel_x = cursor.pixel_x;
  pos.pixel_y = cursor.pixel_y;
  pos.w = tom[0]->w / TILE_W;
  pos.h = tom[0]->h / TILE_H;
  // are we smack on top of a ladder? (extend checking to 1 row below Tom)
  if(pos.pixel_y == 0 && pos.pos_y + pos.h >= map.h) {
    pos.h++;
  }
  ret = containsTypeWhere(&pos, &where, TYPE_LADDER);
  if(ret)
    return (where.pos_y >= pos.pos_y + (tom[0]->h / TILE_H) ? 1 : 2);
  return 0;
}

int
gameDetectSlide()
{
  Position pos;
  if(game.balloonTimer)
    return 0;                   // With a balloon you can move where you want

  pos.pos_x = cursor.pos_x;
  pos.pos_y = cursor.pos_y;
  pos.pixel_x = cursor.pixel_x;
  pos.pixel_y = cursor.pixel_y;
  pos.w = tom[0]->w / TILE_W;
  pos.h = tom[0]->h / TILE_H;
  // are we smack on top of a ladder? (extend checking to 1 row below Tom)
  if(pos.pixel_y == 0 && pos.pos_y + pos.h >= map.h) {
    pos.h++;
  }
  return containsTypeInLevel(&pos, NULL, TYPE_SLIDE, LEVEL_FORE);
}

void
runMap()
{
  int running_savedgame = 0;

  resetMap();
  resetMonsters();

  // try to start where we left off (or load static map if no saved game was found)
  running_savedgame = loadGame();

  if(!running_savedgame) {
    // start outside
    if(GOD_MODE) {
      game.player_start_x = 44;
      game.player_start_y = 148;
    } else {
      game.player_start_x = 20;
      game.player_start_y = 28;
    }

    game.lives = 5;
    game.score = 0;
    game.keys = 0;
    game.balloons = 0;

    // start inside
    cursor.pos_x = game.player_start_x;
    cursor.pos_y = game.player_start_y;
  }

  game.end_game = 0;
  game.draw_player = 1;
  game.balloonTimer = 0;
  cursor.speed_x = 8;
  cursor.speed_y = 8;

  // set our painting events
  map.beforeDrawToScreen = gameBeforeDrawToScreen;
  map.afterMainLevelDrawn = afterMainLevelDrawn;
  map.detectCollision = detectCollision;
  map.detectLadder = detectLadder;
  map.detectSlide = gameDetectSlide;
  map.checkPosition = gameCheckPosition;

  // add our event handling
  map.handleMapEvent = gameMainLoop;

  // activate gravity and accelerated movement
  map.accelerate = 1;
  map.gravity = 1;
  map.monsters = 1;
  map.slides = 1;

  // start the map main loop
  playGameMusic();
  drawMap();
  moveMap();
}

void
initGame()
{
  game.face = 0;
  game.dir = DIR_LEFT;
  game.balloonTimer = 0;
  game.god_mode = GOD_MODE;
  game.lastSavePosX = 0;
  game.lastSavePosY = 0;
  game.health = MAX_HEALTH;
  game.difficoulty = 1;
  game.dir_changed = 0;
  game.in_water = 0;
  game.tick = 0;
}

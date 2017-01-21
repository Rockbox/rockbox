#include "Splash.h"

int menu_y;
#define BULLET_FACE_COUNT 4
int face = 0, face_mod = 4;

char text[] =
  "abes amazing adventure!! code and graphics by gabor music by alex testing by christie inspiration by abe and tom special thanks to jay! michal tomaszkiewicz! pasi orovuo!    2003 april 14 happy birthday abraham!!";
int text_pos = -10000;

void
drawScrollText()
{
  if((int) text_pos < -((int) rb->strlen(text) * (int) FONT_WIDTH)) {
    text_pos = screen->w;
  }
  drawString(screen, text_pos, screen->h - 30, text);
  text_pos -= 4;
}

void
splashMainLoop(SDL_Event * event)
{
  switch (event->type) {
  case SDL_KEYDOWN:
    //  printf("The %s key was pressed! scan=%d\n", SDL_GetKeyName(event->key.keysym.sym), event->key.keysym.scancode);
    switch (event->key.keysym.sym) {
    case SDLK_ESCAPE:
      menu_y = (GOD_MODE ? 4 : 3) * FONT_HEIGHT;
      map.quit = 1;
      break;
    case SDLK_DOWN:
      menu_y += FONT_HEIGHT;
      if(menu_y >= FONT_HEIGHT * (GOD_MODE ? 5 : 4))
        menu_y = 0;
      break;
    case SDLK_UP:
      menu_y -= FONT_HEIGHT;
      if(menu_y < 0)
        menu_y = FONT_HEIGHT * (GOD_MODE ? 4 : 3);
      break;
    case SDLK_RETURN:
    case SDLK_SPACE:
      map.quit = 1;
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
    playSound(MENU_SOUND);
    break;
  }
}

void
splashBeforeDrawToScreen()
{
  SDL_Rect pos;
  int x, y, line;

  x = screen->w / 2 - title->w / 2;
  y = screen->h / 2 - 240;
  if(y < 0)
    y = 0;

  pos.x = x;
  pos.y = y;
  pos.w = title->w;
  pos.h = title->h;
  SDL_BlitSurface(title, NULL, screen, &pos);

  x += 50;
  //y += title->h + FONT_HEIGHT;
  y = 50;
  line = y;

  drawString(screen, x, line, "start game");
  line += FONT_HEIGHT;
#if GOD_MODE
  drawString(screen, x, line, "map editor");
  line += FONT_HEIGHT;
#endif
  drawString(screen, x, line, "settings");
  line += FONT_HEIGHT;
  drawString(screen, x, line, "about");
  line += FONT_HEIGHT;
  drawString(screen, x, line, "exit abe!!");
  line += FONT_HEIGHT;

  face++;
  if(face >= face_mod * BULLET_FACE_COUNT) {
    face = 0;
  }
  pos.x = x - 30;
  pos.y = menu_y + y;
  pos.w = images[img_bullet[0]]->image->w;
  pos.h = images[img_bullet[0]]->image->h;
  SDL_BlitSurface(images[img_bullet[face / face_mod]]->image, NULL, screen,
                  &pos);
  pos.x = x + 155;
  pos.y = menu_y + y;
  pos.w = images[img_bullet[0]]->image->w;
  pos.h = images[img_bullet[0]]->image->h;
  SDL_BlitSurface(images[img_bullet[face / face_mod]]->image, NULL, screen,
                  &pos);
  drawScrollText();
}

void
initIntroMap()
{
  menu_y = 0;

  initMap("intro", 640 / TILE_W, 480 / TILE_H);
  resetMap();
  resetMonsters();

  // try to load the map and quit if you can't find it.
  if(!loadMap(0)) {
    fprintf(stderr, "Can't find map file: %s\n", map.name);
    fflush(stderr);
    return;
  }

  cursor.pos_x = map.w / 2;
  cursor.pos_y = map.h / 2;

  // set our painting events
  map.beforeDrawToScreen = splashBeforeDrawToScreen;
  map.handleMapEvent = splashMainLoop;
  map.moveBackground = 1;

  map.monsters = 1;

  playIntroMusic();
}

void
showIntro()
{
  // handle default runmode
  switch (runmode) {
  case RUNMODE_EDITOR:
    initMap("default", 1000, 1000);
    editMap();
    destroyMap();
    break;
  case RUNMODE_GAME:
    initMap("default", 1000, 1000);
    runMap();
    destroyMap();
    break;
  }

  initIntroMap();
  while(1) {
    cursor.dir = DIR_NONE;
    drawMap();
    // start the map main loop
    moveMap();

    // destroy intro map and load the game
    if(menu_y == 0) {
      destroyMap();
      initMap("default", 1000, 1000);
      runMap();
      destroyMap();
      initIntroMap();
    } else if(GOD_MODE && menu_y == 1 * FONT_HEIGHT) {
      destroyMap();
      initMap("default", 1000, 1000);
      editMap();
      destroyMap();
      initIntroMap();
    } else if(menu_y == (GOD_MODE ? 2 : 1) * FONT_HEIGHT) {
      destroyMap();
      showSettings();
      initIntroMap();
    } else if(menu_y == (GOD_MODE ? 3 : 2) * FONT_HEIGHT) {
      destroyMap();
      showAbout();
      initIntroMap();
    } else if(menu_y == (GOD_MODE ? 4 : 3) * FONT_HEIGHT) {
      return;
    }
  }
}

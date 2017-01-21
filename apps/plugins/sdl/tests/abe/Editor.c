#include "Editor.h"

int make_map = 0;
int slide_start_x = -1;
int slide_start_y = -1;
EditPanel edit_panel;
Cursor editor_cursor;
SDL_Surface *map_screen;
#define SAVE_X 260
#define SAVE_Y 0
#define SAVE_WIDTH 550
#define SAVE_HEIGHT 550

/**
   The editor-specific map drawing event.
   This is called before the map is sent to the screen.
 */
void
makeMap()
{
  if(!(map_screen = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                         (SAVE_WIDTH - SAVE_X) * TILE_W,
                                         SAVE_HEIGHT * TILE_H,
                                         screen->format->BitsPerPixel,
                                         0, 0, 0, 0))) {
    fprintf(stderr, "Error creating surface: %s\n", SDL_GetError());
    fflush(stderr);
    return;
  }
  cursor.pos_x = SAVE_X;
  cursor.pos_y = SAVE_Y;
  make_map = 1;
  drawMap();
}

void
beforeDrawToScreen()
{
  SDL_Rect pos;
  int screen_center_x, screen_center_y;

  // save map images
  if(make_map) {
    if(cursor.pos_y >= SAVE_HEIGHT) {
      make_map = 0;
      SDL_Flip(map_screen);
      fprintf(stderr, "saving map.bmp\n");
      SDL_SaveBMP(map_screen, "map.bmp");
      fprintf(stderr, "done saving map.bmp\n");
      SDL_FreeSurface(map_screen);
    } else {
      pos.x = (cursor.pos_x - SAVE_X) * TILE_W;
      pos.y = cursor.pos_y * TILE_H;
      pos.w = screen->w;
      pos.h = screen->h;
      SDL_BlitSurface(screen, NULL, map_screen, &pos);
      SDL_Flip(map_screen);

      cursor.dir = DIR_UPDATE;
      map.redraw = 1;
      fprintf(stderr, "x=%d y=%d\n", cursor.pos_x, cursor.pos_y);
      if(cursor.pos_x < SAVE_WIDTH) {
        cursor.pos_x += screen->w / TILE_W;
      } else {
        cursor.pos_x = SAVE_X;
        cursor.pos_y += screen->h / TILE_H;
      }
    }
  }
  // draw the cursor
  screen_center_x = (screen->w / TILE_W) / 2;
  screen_center_y = (screen->h / TILE_H) / 2;
  pos.x = screen_center_x * TILE_W;
  pos.y = screen_center_y * TILE_H;
  pos.w = TILE_W;
  pos.h = TILE_H;
  SDL_FillRect(screen, &pos,
               SDL_MapRGBA(screen->format, 0xa0, 0xa0, 0x00, 0x00));

  pos.x = editor_cursor.pos_x * TILE_W;
  pos.y = editor_cursor.pos_y * TILE_H;
  pos.w = TILE_W;
  pos.h = TILE_H;
  SDL_FillRect(screen, &pos,
               SDL_MapRGBA(screen->format, 0x00, 0xa0, 0x00, 0x00));

  // draw the edit panel
  drawEditPanel();
}

void
editorAfterScreenFlipped()
{
}

void
drawSlide(int x1, int y1, int x2, int y2)
{
  int sx, sy, x, y;
  Position pos;

  if(x1 == x2 || y1 == y2) {
    showMapStatus("error cant draw slide");
  }
  // swap
  if(y1 > y2) {
    sx = x1;
    sy = y1;
    x1 = x2;
    y1 = y2;
    x2 = sx;
    y2 = sy;
  }
  x = x1;
  for(y = y1; y < y2; y++) {
    pos.pos_x = x;
    pos.pos_y = y;
    setImageNoCheck(LEVEL_MAIN, x, y, img_slideback);
    if(x1 < x2) {
      // NW -> SE slide
      setImageNoCheck(LEVEL_FORE, x - 1, y, img_slide_right[2]);
      setImageNoCheck(LEVEL_FORE, x, y, img_slide_right[0]);
      setImageNoCheck(LEVEL_FORE, x + 1, y, img_slide_right[1]);
      x++;
    } else {
      // NE -> SW slide
      setImageNoCheck(LEVEL_FORE, x - 1, y, img_slide_left[1]);
      setImageNoCheck(LEVEL_FORE, x, y, img_slide_left[0]);
      setImageNoCheck(LEVEL_FORE, x + 1, y, img_slide_left[2]);
      x--;
    }
  }
  showMapStatus("added slide");
  drawMap();
}

void
drawRect(int x1, int y1, int x2, int y2)
{
  int x, y;
  Position pos;
  SDL_Surface *img;

  img = images[edit_panel.image_index]->image;
  for(y = y1; y < y2; y += img->h / TILE_H) {
    for(x = x1; x < x2; x += img->w / TILE_W) {
      pos.pos_x = x;
      pos.pos_y = y;
      setImagePosition(edit_panel.level, edit_panel.image_index, &pos);
    }
  }
  showMapStatus("added rectangle");
  drawMap();
}

/**
   Main editor event handling
*/
void
editorMainLoop(SDL_Event * event)
{
  int n;
  Position pos;
  switch (event->type) {
  case SDL_MOUSEMOTION:
    editor_cursor.pos_x = event->motion.x / TILE_W;
    editor_cursor.pos_y = event->motion.y / TILE_H;
    if(event->motion.x <= TILE_W) {
      cursor.pos_x--;
      if(cursor.pos_x < 0)
        cursor.pos_x = 0;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
    }
    if(event->motion.x > screen->w - TILE_W) {
      cursor.pos_x++;
      if(cursor.pos_x >= map.w)
        cursor.pos_x = map.w - 1;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
    }
    if(event->motion.y <= TILE_H) {
      cursor.pos_y--;
      if(cursor.pos_y < 0)
        cursor.pos_y = 0;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
    }
    if(event->motion.y > screen->h - EDIT_PANEL_HEIGHT - TILE_H) {
      cursor.pos_y++;
      if(cursor.pos_y >= map.h)
        cursor.pos_y = map.h - 1;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
    }

    break;
  case SDL_MOUSEBUTTONUP:
    switch (event->button.button) {
    case SDL_BUTTON_LEFT:
      pos.pos_x = map.top_left_x + editor_cursor.pos_x;
      pos.pos_y = map.top_left_y + editor_cursor.pos_y;
      setImagePosition(edit_panel.level, edit_panel.image_index, &pos);
      break;
    case SDL_BUTTON_MIDDLE:
      pos.pos_x = map.top_left_x + editor_cursor.pos_x;
      pos.pos_y = map.top_left_y + editor_cursor.pos_y;
      setImagePosition(edit_panel.level, EMPTY_MAP, &pos);
      break;
    case SDL_BUTTON_RIGHT:
      n = selectIcon();
      if(n > -1)
        edit_panel.image_index = n;
      /*
         edit_panel.image_index++;
         if(edit_panel.image_index >= image_count) {
         edit_panel.image_index = 0;
         }
       */
      drawMap();
      break;
    default:
      break;
    }
    break;
  case SDL_KEYDOWN:
    //  printf("The %s key was pressed! scan=%d\n", SDL_GetKeyName(event->key.keysym.sym), event->key.keysym.scancode);
    switch (event->key.keysym.sym) {

    case SDLK_DOWN:
      cursor.pos_y++;
      if(cursor.pos_y >= map.h)
        cursor.pos_y = map.h - 1;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
      break;
    case SDLK_UP:
      cursor.pos_y--;
      if(cursor.pos_y < 0)
        cursor.pos_y = 0;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
      break;
    case SDLK_RIGHT:
      cursor.pos_x++;
      if(cursor.pos_x >= map.w)
        cursor.pos_x = map.w - 1;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
      break;
    case SDLK_LEFT:
      cursor.pos_x--;
      if(cursor.pos_x < 0)
        cursor.pos_x = 0;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
      break;

      /*
         case SDLK_LEFT: 
         cursor.dir = DIR_LEFT;
         break;
         case SDLK_RIGHT: 
         cursor.dir = DIR_RIGHT;
         break;
         case SDLK_UP: 
         cursor.dir = DIR_UP;
         break;
         case SDLK_DOWN: 
         cursor.dir = DIR_DOWN;
         break;
       */
    case SDLK_PAGEDOWN:
      cursor.pos_y += screen->h / TILE_H;
      if(cursor.pos_y >= map.h)
        cursor.pos_y = map.h - 1;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
      break;
    case SDLK_PAGEUP:
      cursor.pos_y -= screen->h / TILE_H;
      if(cursor.pos_y < 0)
        cursor.pos_y = 0;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
      break;
    case SDLK_END:
      cursor.pos_x += screen->w / TILE_W;
      if(cursor.pos_x >= map.w)
        cursor.pos_x = map.w - 1;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
      break;
    case SDLK_HOME:
      cursor.pos_x -= screen->w / TILE_H;
      if(cursor.pos_x < 0)
        cursor.pos_x = 0;
      cursor.dir = DIR_NONE;
      map.redraw = 1;
      break;
    case SDLK_RETURN:
      setImage(edit_panel.level, edit_panel.image_index);
      break;
    case SDLK_i:
      n = selectIcon();
      if(n > -1)
        edit_panel.image_index = n;
      drawMap();
      break;
    case SDLK_DELETE:
    case SDLK_BACKSPACE:
      setImage(edit_panel.level, EMPTY_MAP);
      break;
    case SDLK_1:
      edit_panel.level = LEVEL_BACK;
      drawMap();
      break;
    case SDLK_2:
      edit_panel.level = LEVEL_MAIN;
      drawMap();
      break;
    case SDLK_3:
      edit_panel.level = LEVEL_FORE;
      drawMap();
      break;
    case SDLK_5:
      edit_panel.image_index--;
      if(edit_panel.image_index < 0) {
        edit_panel.image_index = image_count - 1;
      }
      drawMap();
      break;
    case SDLK_6:
      edit_panel.image_index++;
      if(edit_panel.image_index >= image_count) {
        edit_panel.image_index = 0;
      }
      drawMap();
      break;
    case SDLK_7:
      edit_panel.image_index = 0;
      drawMap();
      break;
    case SDLK_l:
      loadMap(1);
      break;
    case SDLK_m:
      makeMap();
      break;
    case SDLK_s:
      saveMap();
      break;
    case SDLK_ESCAPE:
      map.quit = 1;
      break;
    case SDLK_q:
      slide_start_x = map.top_left_x + editor_cursor.pos_x;
      slide_start_y = map.top_left_y + editor_cursor.pos_y;
      showMapStatus("slide started");
      break;
    case SDLK_w:
      if(slide_start_x > -1 && slide_start_y > -1) {
        drawSlide(slide_start_x, slide_start_y,
                  map.top_left_x + editor_cursor.pos_x,
                  map.top_left_y + editor_cursor.pos_y);
      } else {
        showMapStatus("press q first!");
      }
      break;
    case SDLK_e:
      if(slide_start_x > -1 && slide_start_y > -1) {
        drawRect(slide_start_x, slide_start_y,
                 map.top_left_x + editor_cursor.pos_x,
                 map.top_left_y + editor_cursor.pos_y);
      } else {
        showMapStatus("press q first!");
      }
      break;
    default:
      break;
    }
    break;
  case SDL_KEYUP:
    //printf("The %s key was released! scan=%d\n", SDL_GetKeyName(event->key.keysym.sym), event->key.keysym.scancode);
    cursor.dir = DIR_NONE;
    break;
  }
}

void
drawEditPanel()
{
  SDL_Rect rect;
  int level;
  char s[80];

  rect.x = 0;
  rect.y = 0;
  rect.w = edit_panel.image->w;
  rect.h = edit_panel.image->h;
  SDL_FillRect(edit_panel.image, &rect,
               SDL_MapRGBA(screen->format, 0x2, 0x0, 0x0, 0x0));

  rect.x = 0;
  rect.y = 0;
  rect.w = edit_panel.image->w;
  rect.h = 5;
  SDL_FillRect(edit_panel.image, &rect,
               SDL_MapRGBA(screen->format, 0x00, 0x00, 0x90, 0x00));


  // draw which level is being drawn
  for(level = LEVEL_BACK; level < LEVEL_COUNT; level++) {
    rect.x = 10 + (level * 40);
    rect.y = 10;
    rect.w = 20;
    rect.h = 20;
    SDL_FillRect(edit_panel.image, &rect,
                 (level == edit_panel.level ?
                  SDL_MapRGBA(screen->format, 0xff, 0x00, 0x00, 0x00) :
                  SDL_MapRGBA(screen->format, 0xff, 0xff, 0xff, 0x00)));
  }

  // draw which image is used
  rect.x = 200;
  rect.y = 10;
  rect.w = images[edit_panel.image_index]->image->w;
  rect.h = images[edit_panel.image_index]->image->h;
  SDL_BlitSurface(images[edit_panel.image_index]->image, NULL,
                  edit_panel.image, &rect);

  // draw some instructions
  drawString(edit_panel.image, 10, 35, "change level 123");
  drawString(edit_panel.image, 10, 35 + FONT_HEIGHT, "change image 567");
  drawString(edit_panel.image, 10, 35 + FONT_HEIGHT * 2, "slide  q  w");
  drawString(edit_panel.image, 400, 5, "editor");
  drawString(edit_panel.image, 400, 5 + FONT_HEIGHT, "draw enter");
  drawString(edit_panel.image, 400, 5 + FONT_HEIGHT * 2, "erase del");


  // draw it on the screen
  rect.x = 0;
  rect.y = screen->h - edit_panel.image->h;
  rect.w = edit_panel.image->w;
  rect.h = edit_panel.image->h;
  SDL_BlitSurface(edit_panel.image, NULL, screen, &rect);
  //  SDL_Flip(screen);


  rb->snprintf(s, 999, "x%d y%d level%d dir%d", cursor.pos_x, cursor.pos_y,
          edit_panel.level, cursor.dir);
  drawString(screen, 5, 5, s);
  rb->snprintf(s, 999, "spx%d spy%d px%d py%d", cursor.speed_x, cursor.speed_y,
          cursor.pixel_x, cursor.pixel_y);
  drawString(screen, 5, 5 + FONT_HEIGHT, s);
  rb->snprintf(s, 999, "mouse x%d y%d", editor_cursor.pos_x, editor_cursor.pos_y);
  drawString(screen, 5, 5 + FONT_HEIGHT * 2, s);
}

void
resetEditPanel()
{
  edit_panel.level = LEVEL_MAIN;
  edit_panel.image_index = 0;
}

void
initEditor()
{
  //  map = NULL;
  if(!(edit_panel.image = SDL_CreateRGBSurface(SDL_HWSURFACE,
                                               screen->w, EDIT_PANEL_HEIGHT,
                                               screen->format->BitsPerPixel,
                                               0, 0, 0, 0))) {
    fprintf(stderr, "Error creating surface: %s\n", SDL_GetError());
    fflush(stderr);
    return;
  }
  editor_cursor.pos_x = editor_cursor.pos_y = 0;
  make_map = 0;
}

void
editMap()
{
  int i, x, y;
  int step_x, step_y;
  Image *img;

  resetMap();

  // set our painting events
  map.beforeDrawToScreen = beforeDrawToScreen;
  map.afterScreenFlipped = editorAfterScreenFlipped;

  // our event handling
  map.handleMapEvent = editorMainLoop;

  // add some tiles in the background
  img = images[img_back];
  step_x = img->image->w / TILE_W;
  step_y = img->image->h / TILE_H;
  for(y = 0; y < map.h; y += step_y) {
    for(x = 0; x < map.w; x += step_x) {
      i = x + (y * map.w);
      map.image_index[LEVEL_BACK][i] = img_back;
      // make a border
      if(y == 0 || y == map.h - 4 || x == 0 || x == map.w - 4) {
        map.image_index[LEVEL_MAIN][i] = img_rock;
      }
    }
  }

  // reset the cursor
  resetCursor();

  // initialize the edit panel
  resetEditPanel();

  // try to load an existing map
  loadMap(0);
  drawMap();

  // start the map's main loop
  moveMap();
}

#ifndef MAP_H
#define MAP_H

#include "Main.h"
#include "Directories.h"

// Throttle the game speed at 25 fps (for fast machines)
#define FPS_THROTTLE 30

// Tile sizes
#define TILE_W 20
#define TILE_H 20

// The layers of the map
#define LEVEL_BACK 0
#define LEVEL_MAIN 1
#define LEVEL_FORE 2
#define LEVEL_COUNT 3

// Movement directions
#define DIR_NONE 0
#define DIR_UP 1
#define DIR_RIGHT 2
#define DIR_DOWN 4
#define DIR_LEFT 8
#define DIR_UPDATE 32

// These are the extra tiles to draw on the left and top.
// They should be the tile width and height of the biggest
// images used.
#define EXTRA_X 5
#define EXTRA_Y 5

// the speed increments in accelerated mode
#define SPEED_INC_X 1
#define SPEED_INC_Y 1

// the max speed
#define SPEED_MAX_X 10
#define SPEED_MAX_Y 10

// the minimum (starting) speed in accelerated mode
#define START_SPEED_X 6
#define START_SPEED_Y 6

// how many units is in a jump?
#define JUMP_LENGTH 10
#define JUMP_SPEED 12

#define GRAVITY_SPEED 10

// an empty map position
#define EMPTY_MAP 0xffff

// how long to keep a status message up? (about 10sec.)
#define STATUS_TIME 100

typedef struct _map {
  char *name;
  Uint16 w, h;
  Uint16 *image_index[LEVEL_COUNT];
  SDL_Surface *level[LEVEL_COUNT];
  SDL_Surface *transfer;
  int delay;
  // painting callbacks
  void (*beforeDrawToScreen) ();
  void (*afterScreenFlipped) ();
  void (*afterMainLevelDrawn) ();
  int (*detectCollision) (int);
  void (*checkPosition) ();
  // return: 0 - no ladder, 1 - on ladder, can only move down, 2 - on ladder can move up or down
  int (*detectLadder) ();
  int (*detectSlide) ();
  void (*handleMapEvent) (SDL_Event *);
  int accelerate;               // 1 for accelerated movement, 0 otherwise(default)
  int gravity;                  // 1 for gravity, 0 otherwise(default)
  int monsters;                 // 1 for active monsters, 0 otherwise(default)
  int slides;                   // 1 for active slides, 0 otherwise(default)
  int redraw;                   // set to 1 to cause a full map repaint
  int top_left_x, top_left_y;   // where is the tile top left corner of the map?
  int delta;
  int fps_override;
  // the 3d background
  SDL_Surface *background;
  SDL_Surface *background_image;
  int moveBackground;           // if 1 the background scroll artificially
  int max_speed_boost;          // for slow machines, add this to movement speed. (0-10 extra)
  int quit;
  SDL_Surface *overlay;
  char status[80];
  int status_time;
} Map;

// an optimization for looking up monsters etc. multiple times
// fixme: add more object, etc. here
typedef struct _interact {
  int ladder_change;            // did on_ladder change just now?
  int on_ladder;                // are we on a ladder?
} Interact;

extern Cursor cursor;
extern Map map;
extern Interact interact;

// the main map loop routine
void moveMap();

void drawMap();
void setImage(int level, int index);
void setImagePosition(int level, int index, Position * pos);
void setImageNoCheck(int level, int x, int y, int image_index);
int initMap(char *name, int w, int h);
void resetMap();
void destroyMap();
void resetCursor();
void repositionCursor(int tile_x, int tile_y);

// return 1 or 0 if movement in that direction is possible
int moveLeft(int checkCollision);
int moveRight(int checkCollision);
int moveUp(int checkCollision, int platform);
int moveDown(int checkCollision, int platform, int slide);

void lockMap();
void unlockMap();

int startJump();
int startJumpN(int n);
/**
   Does the position contain a tile of this type?
   Return 1 if it does, 0 otherwise.
   Hack: no difference between returning index=0 vs. 0 (not found).
 */
int containsType(Position * p, int type);
// like contains type, but returns the position of the object in ret.
int containsTypeWhere(Position * p, Position * ret, int type);
// like above but can specify a level (send NULL for *ret if not needed.)
int containsTypeInLevel(Position * p, Position * ret, int type, int level);
int onSolidGround(Position * p);
// artificially move the background
void scrollBackground();
void showMapStatus(char *s);
#endif

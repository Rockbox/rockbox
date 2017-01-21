#ifndef COMMON_H
#define COMMON_H

#ifdef WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

typedef struct _position {
  int pos_x, pos_y;             // in tiles
  int pixel_x, pixel_y;         // in pixels
  int w, h;                     // in tiles
} Position;

/**
   A monster instance currently on screen.
 */
typedef struct _liveMonster {
  int pos_x;
  int pos_y;
  int pixel_x;
  int pixel_y;
  int speed_x;
  int speed_y;
  int dir;
  int face;
  struct _monster *monster;
  void *custom;
  int remove_me;
  int child_count;
  struct _liveMonster *parent;
} LiveMonster;

/**
   A monster class.
*/
typedef struct _monster {
  int type;
  char name[40];
  int image_count;
  int image_index[256];
  int start_speed_x;
  int start_speed_y;
  int face_mod;
  void (*moveMonster) (LiveMonster * live);
  void (*drawMonster) (SDL_Rect * pos, LiveMonster * live,
                       SDL_Surface * surface, SDL_Surface * img);
  // return 1 if pos intersects with live's position; 0 otherwise
  int (*detectMonster) (Position * pos, LiveMonster * live);
  // use this to allocate memory for the live->custom field.
  void (*allocCustom) (LiveMonster * live);
  int harmless;
  int random_speed;
  struct _monster *breeds;      // creates this type of creature
  void (*breedMonster) (LiveMonster * live, SDL_Rect * pos);
  int max_children;             // how many spawned children this creature can have active at once
  int damage;
} Monster;

typedef struct _cursor {
  int pos_x, pos_y;
  int pixel_x, pixel_y;
  int speed_x, speed_y;
  int dir;
  int wait;
  int jump;
  int gravity;
  int stepup;
  int slide;
  LiveMonster *platform;
} Cursor;

#endif

#ifndef MONSTER_H
#define MONSTER_H

#include "Main.h"
#include "Map.h"

#define MONSTER_CRAB 0
#define MONSTER_SMASHER 1
#define MONSTER_DEMON 2
#define MONSTER_SMASHER2 3
#define MONSTER_PLATFORM 4
#define MONSTER_PLATFORM2 5
#define MONSTER_SPIDER 6
#define MONSTER_BEAR 7
#define MONSTER_TORCH 8
#define MONSTER_ARROW 9
#define MONSTER_FIRE 10
#define MONSTER_STAR 11
#define MONSTER_BULLET 12
#define MONSTER_CANNON 13
#define MONSTER_CANNON2 14
#define MONSTER_BAT 15
#define MONSTER_GHOST 16
#define MONSTER_END_GAME 17

#define MONSTER_COUNT 18

struct _monster;

// if the intersection of tom and monster is bigger than this number
// it is considered a hit. A MONSTER_COLLISION_FUZZ of 0 is the least
// tolerant and a TILE_W is the most. Doesn't apply to harmless monsters.
#define MONSTER_COLLISION_FUZZ 16

// The extra number of tiles around the screen 
// ,.where monsters are still active
#define MONSTER_EXTRA_X 80
#define MONSTER_EXTRA_Y 60

// The max amount of speed change
#define MAX_RANDOM_SPEED 6.0

extern Monster monsters[256];
extern LiveMonster live_monsters[256];
extern int live_monster_count;
extern int move_monsters;

void initMonsters();
void resetMonsters();
void addMonsterImage(int monster_index, int image_index);
int isMonsterImage(int image_index);

void addLiveMonster(int monster_index, int image_index, int x, int y);
void addLiveMonsterChangeMap(int monster_index, int image_index, int x, int y,
                             int change_map);
void removeAllLiveMonsters();
void drawLiveMonsters(SDL_Surface * surface, int start_x, int start_y);
void debugMonsters();

/**
   Return live monster if there's a one at position pos,
   NULL otherwise.
 */
LiveMonster *detectMonster(Position * pos);

#endif

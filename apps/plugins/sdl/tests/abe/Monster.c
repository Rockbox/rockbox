#include "Monster.h"

Monster monsters[256];
LiveMonster live_monsters[256];
int live_monster_count;
int move_monsters;

int getLiveMonsterFace(LiveMonster * live);

SDL_Rect extended_screen_rect;

void
defaultMoveMonster(LiveMonster * live)
{
  // no-op
}

void
defaultDrawMonster(SDL_Rect * pos, LiveMonster * live, SDL_Surface * surface,
                   SDL_Surface * img)
{
  SDL_BlitSurface(img, NULL, surface, pos);
}

void
defaultBreedMonster(LiveMonster * live, SDL_Rect * pos)
{
  // no-op
}

void
initMonsterPos(Position * pos, LiveMonster * live)
{
  pos->pos_x = live->pos_x;
  pos->pos_y = live->pos_y;
  pos->pixel_x = live->pixel_x;
  pos->pixel_y = live->pixel_y;
  pos->w = images[live->monster->image_index[0]]->image->w / TILE_W;
  pos->h = images[live->monster->image_index[0]]->image->h / TILE_H;
}

int
stepMonsterLeft(LiveMonster * live, int float_ok)
{
  Position pos;
  int fail = 0;
  LiveMonster old;
  memcpy(&old, live, sizeof(LiveMonster));
  live->pixel_x -= live->speed_x + map.max_speed_boost;
  if(live->pixel_x < 0) {
    live->pos_x--;
    live->pixel_x = TILE_W + live->pixel_x;
    if(live->pos_x < 0) {
      fail = 1;
    }
  }
  // collision detection
  if(!fail) {
    initMonsterPos(&pos, live);
    if(containsType(&pos, TYPE_WALL | TYPE_DOOR)
       || !(float_ok || onSolidGround(&pos)))
      fail = 1;
  }
  if(fail) {
    memcpy(live, &old, sizeof(LiveMonster));
    return 0;
  }
  return 1;
}

int
stepMonsterUp(LiveMonster * live)
{
  Position pos;
  int fail = 0;
  LiveMonster old;
  memcpy(&old, live, sizeof(LiveMonster));
  live->pixel_y -= live->speed_y + map.max_speed_boost;
  if(live->pixel_y < 0) {
    live->pos_y--;
    live->pixel_y = TILE_H + live->pixel_y;
    if(live->pos_y < 0) {
      fail = 1;
    }
  }
  // collision detection
  if(!fail) {
    initMonsterPos(&pos, live);
    if(containsType(&pos, TYPE_WALL | TYPE_DOOR))
      fail = 1;
  }
  if(fail) {
    memcpy(live, &old, sizeof(LiveMonster));
    return 0;
  }
  return 1;
}

int
stepMonsterRight(LiveMonster * live, int float_ok)
{
  Position pos;
  int fail = 0;
  LiveMonster old;
  memcpy(&old, live, sizeof(LiveMonster));
  live->pixel_x += live->speed_x + map.max_speed_boost;
  if(live->pixel_x >= TILE_W) {
    live->pos_x++;
    live->pixel_x = live->pixel_x - TILE_W;
    if(live->pos_x >= map.w) {
      fail = 1;
    }
  }
  // collision detection
  if(!fail) {
    initMonsterPos(&pos, live);
    if(containsType(&pos, TYPE_WALL | TYPE_DOOR)
       || !(float_ok || onSolidGround(&pos)))
      fail = 1;
  }
  if(fail) {
    memcpy(live, &old, sizeof(LiveMonster));
    return 0;
  }
  return 1;
}

int
stepMonsterDown(LiveMonster * live)
{
  Position pos;
  int fail = 0;
  LiveMonster old;

  memcpy(&old, live, sizeof(LiveMonster));

  live->pixel_y += live->speed_y + map.max_speed_boost;
  if(live->pixel_y >= TILE_H) {
    live->pos_y++;
    live->pixel_y = live->pixel_y - TILE_H;
    if(live->pos_y >= map.h)
      fail = 1;
  }
  // collision detection
  if(!fail) {
    initMonsterPos(&pos, live);
    if(containsType(&pos, TYPE_WALL | TYPE_DOOR))
      fail = 1;
  }

  if(fail) {
    memcpy(live, &old, sizeof(LiveMonster));
    return 0;
  }
  return 1;
}

void
moveDemon(LiveMonster * live_monster)
{
  int j;

  // move sideways until you hit a wall or an edge
  j = 1 + (int) (10.0 * rand() / (RAND_MAX));
  if(j > 7) {
    // increment the face to display
    live_monster->face++;
    if(live_monster->face >=
       live_monster->monster->image_count * live_monster->monster->face_mod)
      live_monster->face = 0;

    if(live_monster->dir == DIR_LEFT) {
      if(!stepMonsterLeft(live_monster, 0)) {
        live_monster->dir = DIR_RIGHT;
      }
    } else {
      if(!stepMonsterRight(live_monster, 0)) {
        live_monster->dir = DIR_LEFT;
      }
    }
  }
}

void
movePlatform(LiveMonster * live_monster)
{
  // increment the face to display
  live_monster->face++;
  if(live_monster->face >=
     live_monster->monster->image_count * live_monster->monster->face_mod)
    live_monster->face = 0;

  // move sideways until you hit a wall or an edge
  if(live_monster->dir == DIR_LEFT) {
    if(!stepMonsterLeft(live_monster, 1)) {
      live_monster->dir = DIR_RIGHT;
    }
  } else {
    if(!stepMonsterRight(live_monster, 1)) {
      live_monster->dir = DIR_LEFT;
    }
  }
}

void
movePlatform2(LiveMonster * live_monster)
{
  // increment the face to display
  live_monster->face++;
  if(live_monster->face >=
     live_monster->monster->image_count * live_monster->monster->face_mod)
    live_monster->face = 0;

  // move up and down
  if(live_monster->dir == DIR_DOWN) {
    if(!stepMonsterDown(live_monster)) {
      live_monster->dir = DIR_UP;
    }
  } else {
    if(!stepMonsterUp(live_monster)) {
      live_monster->dir = DIR_DOWN;
    }
  }
}

void
moveEndGame(LiveMonster * live_monster)
{
  int *p;
  p = (int *) (live_monster->custom);
  // if our custom int is set
  if(*p > 0) {
    // move up and down
    if(live_monster->dir == DIR_DOWN) {
      if(!stepMonsterDown(live_monster)) {
        live_monster->dir = DIR_UP;
      }
    } else {
      if(!stepMonsterUp(live_monster)) {
        live_monster->dir = DIR_DOWN;
      } else {
        if((*p)++ >= 2) {
          *p = 1;
          live_monster->dir = DIR_DOWN;
        }
      }
    }
  }
}

void
moveCrab(LiveMonster * live_monster)
{
  // increment the face to display
  live_monster->face++;
  if(live_monster->face >=
     live_monster->monster->image_count * live_monster->monster->face_mod)
    live_monster->face = 0;

  // move sideways until you hit a wall or an edge
  if(live_monster->dir == DIR_LEFT) {
    if(!stepMonsterLeft(live_monster, 0)) {
      live_monster->dir = DIR_RIGHT;
    }
  } else {
    if(!stepMonsterRight(live_monster, 0)) {
      live_monster->dir = DIR_LEFT;
    }
  }
}

void
moveGhost(LiveMonster * live_monster)
{
  int j;
  // increment the face to display
  j = (int) (10.0 * rand() / (RAND_MAX));
  if(!j) {
    live_monster->face++;
    if(live_monster->face >=
       live_monster->monster->image_count * live_monster->monster->face_mod)
      live_monster->face = 0;
  }
  // move up/down until you hit a wall or an edge
  if(live_monster->dir == DIR_UP) {
    if(!stepMonsterUp(live_monster)) {
      live_monster->dir = DIR_DOWN;
    }
  } else {
    if(!stepMonsterDown(live_monster)) {
      live_monster->dir = DIR_UP;
    }
  }
}

void
moveBat(LiveMonster * live_monster)
{
  int j;

  // increment the face to display
  live_monster->face++;
  if(live_monster->face >=
     live_monster->monster->image_count * live_monster->monster->face_mod)
    live_monster->face = 0;

  if(live_monster->dir == DIR_UPDATE) {
    j = (int) (30.0 * rand() / (RAND_MAX));
    if(j == 0) {
      live_monster->dir = DIR_LEFT;
      if(!stepMonsterLeft(live_monster, 1)) {
        live_monster->dir = DIR_RIGHT;
        stepMonsterRight(live_monster, 1);
      }
    }
  } else {
    // move sideways until you hit a wall or an edge
    if(live_monster->pos_y % 6 < 3)
      stepMonsterDown(live_monster);
    else
      stepMonsterUp(live_monster);

    if(live_monster->dir == DIR_LEFT) {
      if(!stepMonsterLeft(live_monster, 1)) {
        live_monster->dir = DIR_UPDATE;
      }
    } else {
      if(!stepMonsterRight(live_monster, 1)) {
        live_monster->dir = DIR_UPDATE;
      }
    }
  }
}

void
breedBullet(LiveMonster * live, SDL_Rect * pos)
{
  addLiveMonsterChangeMap(MONSTER_BULLET, img_bullet[0], live->pos_x + 2,
                          live->pos_y, 0);
  live_monsters[live_monster_count - 1].pixel_x = 10;
  live_monsters[live_monster_count - 1].dir = DIR_RIGHT;
  live_monsters[live_monster_count - 1].parent = live;
  live->child_count++;
}

void
breedBullet2(LiveMonster * live, SDL_Rect * pos)
{
  addLiveMonsterChangeMap(MONSTER_BULLET, img_bullet[0], live->pos_x - 1,
                          live->pos_y, 0);
  live_monsters[live_monster_count - 1].dir = DIR_LEFT;
  live_monsters[live_monster_count - 1].parent = live;
  live->child_count++;
}

void
moveBullet(LiveMonster * live_monster)
{
  // increment the face to display
  live_monster->face++;
  if(live_monster->face >=
     live_monster->monster->image_count * live_monster->monster->face_mod)
    live_monster->face = 0;

  // move sideways until you hit a wall
  if(live_monster->dir == DIR_LEFT) {
    if(!stepMonsterLeft(live_monster, 1)) {
      live_monster->remove_me = 1;
    }
  } else {
    if(!stepMonsterRight(live_monster, 1)) {
      live_monster->remove_me = 1;
    }
  }
}

void
moveArrow(LiveMonster * live_monster)
{
  // increment the face to display
  int old = getLiveMonsterFace(live_monster);
  int face;
  int n = (int) (100.0 * rand() / (RAND_MAX + 1.0));
  if(n > 0)
    return;

  live_monster->face++;
  if(live_monster->face >=
     live_monster->monster->image_count * live_monster->monster->face_mod) {
    live_monster->face = 0;
  }
  face = getLiveMonsterFace(live_monster);
  if(old < face) {
    live_monster->pos_y--;
  } else if(old > face) {
    live_monster->pos_y++;
  }
}

void
moveTorch(LiveMonster * live_monster)
{
  // increment the face to display
  live_monster->face++;
  if(live_monster->face >=
     live_monster->monster->image_count * live_monster->monster->face_mod)
    live_monster->face = 0;
}

void
moveBear(LiveMonster * live_monster)
{
  // increment the face to display
  int n = live_monster->monster->image_count / 2;
  live_monster->face++;
  // move sideways until you hit a wall or an edge
  if(live_monster->dir == DIR_LEFT) {
    if(live_monster->face >= n * live_monster->monster->face_mod)
      live_monster->face = 0;
    if(!stepMonsterLeft(live_monster, 0)) {
      live_monster->dir = DIR_RIGHT;
      live_monster->face = 0;
    }
  } else {
    if(live_monster->face >=
       live_monster->monster->image_count * live_monster->monster->face_mod)
      live_monster->face = n * live_monster->monster->face_mod;
    if(!stepMonsterRight(live_monster, 0)) {
      live_monster->dir = DIR_LEFT;
      live_monster->face = n * live_monster->monster->face_mod;
    }
  }
}

void
moveFire(LiveMonster * live_monster)
{
  // move up and down until you hit an edge
  if(live_monster->dir == DIR_DOWN) {
    if(!stepMonsterDown(live_monster)) {
      live_monster->dir = DIR_UP;
      live_monster->speed_y =
        live_monster->monster->start_speed_y +
        ((int) ((MAX_RANDOM_SPEED / 2) * rand() / (RAND_MAX)));
      if(live_monster->speed_y <= 0)
        live_monster->speed_y = 1;
    }
  } else {
    if(!stepMonsterUp(live_monster)) {
      live_monster->dir = DIR_DOWN;
      live_monster->speed_y =
        live_monster->monster->start_speed_y +
        ((int) ((MAX_RANDOM_SPEED / 2) * rand() / (RAND_MAX)));
      if(live_monster->speed_y <= 0)
        live_monster->speed_y = 1;
    }
  }
}

void
drawFire(SDL_Rect * pos, LiveMonster * live, SDL_Surface * surface,
         SDL_Surface * img)
{
  int y, original;
  SDL_Rect p, q;
  Position position;
  int index;

  p.x = pos->x;
  //  p.y = (pos->y / TILE_H) * TILE_H - TILE_H;
  original = y = p.y = pos->y;
  p.w = pos->w;
  p.h = pos->h;

  position.pos_x = live->pos_x;
  position.pos_y = live->pos_y;
  position.pixel_x = live->pixel_x;
  position.pixel_y = live->pixel_y;
  position.w = p.w / TILE_W;
  position.h = p.h / TILE_H;

  while(position.pos_y < map.h &&
        !containsType(&position, TYPE_WALL | TYPE_DOOR)) {
    index = (int) ((double) (live->monster->image_count) * rand() / RAND_MAX);
    SDL_BlitSurface(images[live->monster->image_index[index]]->image, NULL,
                    surface, &p);
    p.x = pos->x;
    y += TILE_H;
    p.y = y;
    p.w = pos->w;
    p.h = pos->h;
    position.pos_y++;
  }

  // save the fire column's height in the custom field
  *((int *) (live->custom)) = y - original;

  // draw the last one
  if(position.pos_y > live->pos_y && live->pixel_y) {
    index = (int) ((double) (live->monster->image_count) * rand() / RAND_MAX);
    q.x = 0;
    q.y = 0;
    q.w = images[live->monster->image_index[index]]->image->w;
    q.h = images[live->monster->image_index[index]]->image->h - live->pixel_y;
    SDL_BlitSurface(images[live->monster->image_index[index]]->image, &q,
                    surface, &p);
  }
}

void
moveSmasher(LiveMonster * live_monster)
{
  // move up and down until you hit an edge
  if(live_monster->dir == DIR_DOWN) {
    if(!stepMonsterDown(live_monster)) {
      live_monster->dir = DIR_UP;
      live_monster->speed_y =
        live_monster->monster->start_speed_y / 2 +
        ((int) ((MAX_RANDOM_SPEED / 2) * rand() / (RAND_MAX)));
      if(live_monster->speed_y <= 0)
        live_monster->speed_y = 1;
    }
  } else {
    if(!stepMonsterUp(live_monster)) {
      live_monster->dir = DIR_DOWN;
      live_monster->speed_y =
        live_monster->monster->start_speed_y +
        ((int) ((MAX_RANDOM_SPEED / 2) * rand() / (RAND_MAX)));
      if(live_monster->speed_y <= 0)
        live_monster->speed_y = 1;
    }
  }
}

void
drawSmasher(SDL_Rect * pos, LiveMonster * live, SDL_Surface * surface,
            SDL_Surface * img)
{
  int y = 0;
  SDL_Rect p, q;
  Position position;
  int first_image, first, second;
  int skip;

  first_image = live->monster->image_index[0];
  first = (first_image == img_smash || first_image == img_smash2 ? img_smash :
           (first_image == img_smash3
            || first_image == img_smash4 ? img_smash3 : img_spider));
  second =
    (first ==
     img_smash ? img_smash2 : (first ==
                               img_smash3 ? img_smash4 : img_spider2));

  p.x = pos->x;
  //  p.y = (pos->y / TILE_H) * TILE_H - TILE_H;
  p.y = pos->y - TILE_H;
  p.w = pos->w;
  p.h = pos->h;

  position.pos_x = live->pos_x;
  position.pos_y = live->pos_y - 1;
  position.pixel_x = 0;
  position.pixel_y = 0;
  position.w = p.w / TILE_W;
  position.h = p.h / TILE_H;

  skip = 0;
  while(position.pos_y >= 0 &&
        !containsType(&position, TYPE_WALL | TYPE_DOOR)) {
    SDL_BlitSurface(images[second]->image, NULL, surface, &p);
    // HACK part 1: if p->y is reset to 0 the image was cropped.
    y = p.y;
    if(!y) {
      skip = 1;
      break;
    }
    p.x = pos->x;
    p.y -= TILE_H;
    p.w = pos->w;
    p.h = pos->h;
    position.pos_y--;
  }
  // draw the top one.
  // HACK part 2: if y is 0 the image was cropped, don't draw
  //  if(y && live->pixel_y) {
  if(!skip && live->pixel_y != 0) {
    p.x = pos->x;
    p.y += TILE_H;
    p.y -= live->pixel_y;
    p.w = pos->w;
    p.h = pos->h;

    q.x = 0;
    q.y = TILE_H - live->pixel_y;
    q.w = p.w;
    q.h = images[second]->image->h - q.y;
    SDL_BlitSurface(images[second]->image, &q, surface, &p);
  }

  SDL_BlitSurface(images[first]->image, NULL, surface, pos);
}

int
defaultDetectMonster(Position * pos, LiveMonster * live)
{
  SDL_Rect monster, check;
  SDL_Surface *img;

  // convert pos to pixels
  check.x = pos->pos_x * TILE_W + pos->pixel_x;
  check.y = pos->pos_y * TILE_H + pos->pixel_y;
  check.w = pos->w * TILE_W;
  check.h = pos->h * TILE_H;

  // get the live monster's pixel rect.
  img = images[live->monster->image_index[getLiveMonsterFace(live)]]->image;
  monster.x = live->pos_x * TILE_W + live->pixel_x;
  monster.y = live->pos_y * TILE_H + live->pixel_y;
  monster.w = img->w;
  monster.h = img->h;

  // compare
  return intersectsBy(&check, &monster,
                      (live->monster->harmless ? 1 : MONSTER_COLLISION_FUZZ));
}

int
detectFire(Position * pos, LiveMonster * live)
{
  SDL_Rect monster, check;

  // convert pos to pixels
  check.x = pos->pos_x * TILE_W + pos->pixel_x;
  check.y = pos->pos_y * TILE_H + pos->pixel_y;
  check.w = pos->w * TILE_W;
  check.h = pos->h * TILE_H;

  // get the live monster's pixel rect.
  //  img = images[live->monster->image_index[getLiveMonsterFace(live)]]->image;
  monster.x = live->pos_x * TILE_W + live->pixel_x;
  monster.y = live->pos_y * TILE_H + live->pixel_y;
  monster.w = TILE_W;
  monster.h = *((int *) (live->custom));

  // compare
  return intersectsBy(&check, &monster,
                      (live->monster->harmless ? 1 : MONSTER_COLLISION_FUZZ));
}

void
allocFireCustom(LiveMonster * live)
{
//warning: use of cast expressions as lvalues is deprecated
//if(((int *) live->custom = (int *) malloc(sizeof(int))) == NULL) {
  if((live->custom = (int *) malloc(sizeof(int))) == NULL) {
    printf(
            "Out of memory when trying to allocate custom storage for fire column.\n");
  }
}

void
allocEndGameCustom(LiveMonster * live)
{
//if(((int *) live->custom = (int *) malloc(sizeof(int))) == NULL) {
  if((live->custom = (int *) malloc(sizeof(int))) == NULL) {
    printf(
            "Out of memory when trying to allocate custom storage for end game.\n");
  }
  *((int *) (live->custom)) = 0;
}

/**
   Remember here, images are not yet initialized!
 */
void
initMonsters()
{
  int i;

  move_monsters = 1;
  live_monster_count = 0;

  // init the screen rectangle.
  extended_screen_rect.x = -MONSTER_EXTRA_X * TILE_W;
  extended_screen_rect.y = -MONSTER_EXTRA_Y * TILE_H;
  extended_screen_rect.w = screen->w + 2 * MONSTER_EXTRA_X * TILE_W;
  extended_screen_rect.h = screen->h + 2 * MONSTER_EXTRA_Y * TILE_H;

  // common properties.
  for(i = 0; i < MONSTER_COUNT; i++) {
    monsters[i].start_speed_x = 2;
    monsters[i].start_speed_y = 2;
    monsters[i].image_count = 0;
    monsters[i].moveMonster = defaultMoveMonster;
    monsters[i].drawMonster = defaultDrawMonster;
    monsters[i].face_mod = 1;
    monsters[i].type = i;
    monsters[i].harmless = 0;
    monsters[i].random_speed = 1;
    monsters[i].detectMonster = defaultDetectMonster;
    monsters[i].allocCustom = NULL;
    monsters[i].breeds = NULL;
    monsters[i].breedMonster = defaultBreedMonster;
    monsters[i].damage = 1;
  }

  // crab monster
  strcpy(monsters[MONSTER_CRAB].name, "dungenous crab");
  monsters[MONSTER_CRAB].moveMonster = moveCrab;
  monsters[MONSTER_CRAB].start_speed_x = 2;
  monsters[MONSTER_CRAB].start_speed_y = 2;
  // animation 2x slower
  monsters[MONSTER_CRAB].face_mod = 2;
  monsters[MONSTER_CRAB].random_speed = 0;

  // smasher monster
  strcpy(monsters[MONSTER_SMASHER].name, "smasher");
  monsters[MONSTER_SMASHER].moveMonster = moveSmasher;
  monsters[MONSTER_SMASHER].drawMonster = drawSmasher;
  monsters[MONSTER_SMASHER].start_speed_x = 4;
  monsters[MONSTER_SMASHER].start_speed_y = 4;
  monsters[MONSTER_SMASHER].damage = 2;

  // purple smasher
  strcpy(monsters[MONSTER_SMASHER2].name, "cursed smasher");
  monsters[MONSTER_SMASHER2].moveMonster = moveSmasher;
  monsters[MONSTER_SMASHER2].drawMonster = drawSmasher;
  monsters[MONSTER_SMASHER2].start_speed_x = 4;
  monsters[MONSTER_SMASHER2].start_speed_y = 4;
  monsters[MONSTER_SMASHER2].damage = 4;

  // demon monster
  strcpy(monsters[MONSTER_DEMON].name, "little demon");
  monsters[MONSTER_DEMON].moveMonster = moveDemon;
  monsters[MONSTER_DEMON].start_speed_x = 4;
  monsters[MONSTER_DEMON].start_speed_y = 4;
  // animation 2x slower
  monsters[MONSTER_DEMON].face_mod = 4;
  monsters[MONSTER_DEMON].damage = 2;

  // platforms
  strcpy(monsters[MONSTER_PLATFORM].name, "platform");
  monsters[MONSTER_PLATFORM].moveMonster = movePlatform;
  monsters[MONSTER_PLATFORM].start_speed_x = 4;
  monsters[MONSTER_PLATFORM].start_speed_y = 4;
  monsters[MONSTER_PLATFORM].harmless = 1;

  // platform2
  strcpy(monsters[MONSTER_PLATFORM2].name, "platform2");
  monsters[MONSTER_PLATFORM2].moveMonster = movePlatform2;
  monsters[MONSTER_PLATFORM2].start_speed_x = 4;
  monsters[MONSTER_PLATFORM2].start_speed_y = 4;
  monsters[MONSTER_PLATFORM2].harmless = 1;

  // spider
  strcpy(monsters[MONSTER_SPIDER].name, "patient spider");
  monsters[MONSTER_SPIDER].moveMonster = moveSmasher;
  monsters[MONSTER_SPIDER].drawMonster = drawSmasher;
  monsters[MONSTER_SPIDER].start_speed_x = 2;
  monsters[MONSTER_SPIDER].start_speed_y = 2;
  monsters[MONSTER_SPIDER].damage = 5;

  // bear monster
  strcpy(monsters[MONSTER_BEAR].name, "arctic cave bear");
  monsters[MONSTER_BEAR].moveMonster = moveBear;
  monsters[MONSTER_BEAR].start_speed_x = 1;
  monsters[MONSTER_BEAR].start_speed_y = 1;
  monsters[MONSTER_BEAR].face_mod = 8;
  monsters[MONSTER_BEAR].random_speed = 0;
  monsters[MONSTER_BEAR].damage = 7;

  // torch
  strcpy(monsters[MONSTER_TORCH].name, "torch");
  monsters[MONSTER_TORCH].moveMonster = moveTorch;
  monsters[MONSTER_TORCH].start_speed_x = 1;
  monsters[MONSTER_TORCH].start_speed_y = 1;
  monsters[MONSTER_TORCH].face_mod = 6;
  monsters[MONSTER_TORCH].harmless = 1;

  // arrow trap
  strcpy(monsters[MONSTER_ARROW].name, "arrow trap");
  monsters[MONSTER_ARROW].moveMonster = moveArrow;
  monsters[MONSTER_ARROW].start_speed_x = 1;
  monsters[MONSTER_ARROW].start_speed_y = 1;
  monsters[MONSTER_ARROW].face_mod = 1;
  monsters[MONSTER_ARROW].damage = 1;

  // fire
  strcpy(monsters[MONSTER_FIRE].name, "fire trap");
  monsters[MONSTER_FIRE].moveMonster = moveFire;
  monsters[MONSTER_FIRE].drawMonster = drawFire;
  monsters[MONSTER_FIRE].start_speed_x = 2;
  monsters[MONSTER_FIRE].start_speed_y = 2;
  monsters[MONSTER_FIRE].face_mod = 3;
  monsters[MONSTER_FIRE].detectMonster = detectFire;
  monsters[MONSTER_FIRE].allocCustom = allocFireCustom;
  monsters[MONSTER_FIRE].damage = 4;

  // star
  strcpy(monsters[MONSTER_STAR].name, "star");
  monsters[MONSTER_STAR].moveMonster = moveTorch; // re-use moveTorch; not a bug
  monsters[MONSTER_STAR].start_speed_x = 1;
  monsters[MONSTER_STAR].start_speed_y = 1;
  monsters[MONSTER_STAR].face_mod = 4;
  monsters[MONSTER_STAR].harmless = 1;

  // bullet
  strcpy(monsters[MONSTER_BULLET].name, "curious cannonball");
  monsters[MONSTER_BULLET].moveMonster = moveBullet;
  monsters[MONSTER_BULLET].start_speed_x = 8;
  monsters[MONSTER_BULLET].start_speed_y = 4;
  monsters[MONSTER_BULLET].face_mod = 1;
  monsters[MONSTER_BULLET].random_speed = 0;
  monsters[MONSTER_BULLET].damage = 10;

  // cannon
  strcpy(monsters[MONSTER_CANNON].name, "cannon");
  monsters[MONSTER_CANNON].harmless = 1;
  monsters[MONSTER_CANNON].breeds = &monsters[MONSTER_BULLET];
  monsters[MONSTER_CANNON].breedMonster = breedBullet;
  monsters[MONSTER_CANNON].max_children = 1;  // 1 bullet active at a time

  // cannon2
  strcpy(monsters[MONSTER_CANNON2].name, "cannon");
  monsters[MONSTER_CANNON2].harmless = 1;
  monsters[MONSTER_CANNON2].breeds = &monsters[MONSTER_BULLET];
  monsters[MONSTER_CANNON2].breedMonster = breedBullet2;
  monsters[MONSTER_CANNON2].max_children = 1; // 1 bullet active at a time

  // bat
  strcpy(monsters[MONSTER_BAT].name, "vampire bat");
  monsters[MONSTER_BAT].moveMonster = moveBat;
  monsters[MONSTER_BAT].start_speed_x = 6;
  monsters[MONSTER_BAT].start_speed_y = 4;
  monsters[MONSTER_BAT].face_mod = 3;
  monsters[MONSTER_BAT].damage = 3;

  // ghost
  strcpy(monsters[MONSTER_GHOST].name, "entombed spirit");
  monsters[MONSTER_GHOST].moveMonster = moveGhost;
  monsters[MONSTER_GHOST].start_speed_y = 2;
  monsters[MONSTER_GHOST].damage = 2;
  monsters[MONSTER_GHOST].face_mod = 4;
  monsters[MONSTER_GHOST].random_speed = 0;

  // end game
  strcpy(monsters[MONSTER_END_GAME].name, "lost friend!");
  monsters[MONSTER_END_GAME].harmless = 1;
  monsters[MONSTER_END_GAME].moveMonster = moveEndGame;
  monsters[MONSTER_END_GAME].start_speed_y = 3;
  monsters[MONSTER_END_GAME].allocCustom = allocEndGameCustom;
  monsters[MONSTER_END_GAME].random_speed = 0;

  // add additional monsters here

  for(i = 0; i < MONSTER_COUNT; i++) {
    printf("Added monster: %s.\n", monsters[i].name);
  }
  fflush(stderr);
}

void
resetMonsters()
{
  live_monster_count = 0;
}

void
addMonsterImage(int monster_index, int image_index)
{
  monsters[monster_index].image_index[monsters[monster_index].image_count++] =
    image_index;
  printf("monster image added. monster=%d image_count=%d\n",
          monster_index, monsters[monster_index].image_count);
  fflush(stderr);
}

int
isMonsterImage(int image_index)
{
  // new fast way of doing this.
  if(image_index == EMPTY_MAP)
    return -1;
  return images[image_index]->monster_index;
}

void
addLiveMonster(int monster_index, int image_index, int x, int y)
{
  addLiveMonsterChangeMap(monster_index, image_index, x, y, 1);
}

void
addLiveMonsterChangeMap(int monster_index, int image_index, int x, int y,
                        int change_map)
{
  int i;
  Monster *m = &monsters[monster_index];
  live_monsters[live_monster_count].parent = NULL;
  live_monsters[live_monster_count].pos_x = x;
  live_monsters[live_monster_count].pos_y = y;
  live_monsters[live_monster_count].pixel_x = 0;
  live_monsters[live_monster_count].pixel_y = 0;
  if(m->random_speed) {
    live_monsters[live_monster_count].speed_x =
      m->start_speed_x + ((int) (MAX_RANDOM_SPEED * rand() / (RAND_MAX)));
    live_monsters[live_monster_count].speed_y =
      m->start_speed_y + ((int) (MAX_RANDOM_SPEED * rand() / (RAND_MAX)));
  } else {
    live_monsters[live_monster_count].speed_x = m->start_speed_x;
    live_monsters[live_monster_count].speed_y = m->start_speed_y;
  }
  live_monsters[live_monster_count].dir = DIR_NONE;
  live_monsters[live_monster_count].face = 0;
  for(i = 0; i < m->image_count; i++) {
    if(m->image_index[i] == image_index) {
      live_monsters[live_monster_count].face = i;
    }
  }
  live_monsters[live_monster_count].remove_me = 0;
  live_monsters[live_monster_count].monster = m;
  live_monsters[live_monster_count].child_count = 0;
  // allocate custom storage
  if(live_monsters[live_monster_count].monster->allocCustom) {
    live_monsters[live_monster_count].monster->
      allocCustom(&live_monsters[live_monster_count]);
  }
  live_monster_count++;

  // remove image from map
  if(change_map) {
    map.image_index[LEVEL_MAIN][x + (y * map.w)] = EMPTY_MAP;
  }
}

void
removeLiveMonster(int live_monster_index)
{
  int i, t;
  LiveMonster *p;

  // debug
  if(live_monster_index >= live_monster_count) {
    printf(
            "Trying to remove monster w. index out of bounds: count=%d index=%d\n",
            live_monster_count, live_monster_index);
    for(t = 0; t < live_monster_count; t++) {
      printf("\tmonster=%s x=%d y=%d\n",
              live_monsters[t].monster->name, live_monsters[t].pos_x,
              live_monsters[t].pos_y);
    }
    fflush(stderr);
    exit(-1);
  }
  // add it back to the map
  p = &live_monsters[live_monster_index];
  // If the monster was bred (bullets) it doesn't need to be saved in the map.
  if(!p->parent) {
    setImageNoCheck(LEVEL_MAIN,
                    p->pos_x, p->pos_y,
                    p->monster->image_index[getLiveMonsterFace(p)]);
    // remove its children
    for(t = 0; p->child_count && t < live_monster_count; t++) {
      if(live_monsters[t].parent == p) {
        removeLiveMonster(t);
        t--;
      }
    }
  } else {
    p->parent->child_count--;
  }

  // free custom storage
  if(live_monsters[live_monster_index].monster->allocCustom) {
    free(live_monsters[live_monster_index].custom);
  }
  // remove it from memory
  for(t = live_monster_index; t < live_monster_count - 1; t++) {
    for(i = 0; i < live_monster_count; i++) {
      if(live_monsters[i].parent == &live_monsters[t + 1]) {
        live_monsters[i].parent = &live_monsters[t];
      }
    }
    memcpy(&live_monsters[t], &live_monsters[t + 1], sizeof(LiveMonster));
  }
  live_monster_count--;
}

void
debugMonsters()
{
  int t, i, n;
  printf("Monsters:\n");
  for(t = 0; t < live_monster_count; t++) {
    n = -1;
    for(i = 0; live_monsters[t].parent && i < live_monster_count; i++) {
      if(&live_monsters[i] == live_monsters[t].parent) {
        n = i;
        break;
      }
    }
    printf(
            "\t%d monster=%s x=%d y=%d child_count=%d remove_me=%d parent=%d\n",
            t, live_monsters[t].monster->name, live_monsters[t].pos_x,
            live_monsters[t].pos_y, live_monsters[t].child_count,
            live_monsters[t].remove_me, n);
  }
  fflush(stderr);
}

void
removeAllLiveMonsters()
{
  while(live_monster_count > 0) {
    removeLiveMonster(0);
  }
}

/**
   Here rect is in pixels where 0, 0 is the screen's left top corner.
   Returns 0 for false and non-0 for true.
 */
int
isOnScreen(SDL_Rect * rect)
{
  return intersects(rect, &extended_screen_rect);
}

/**
   Draw all currently tracked creatures.
   start_x, start_y are the offset of the screen's top left edge in pixels.
 */
void
drawLiveMonsters(SDL_Surface * surface, int start_x, int start_y)
{
  SDL_Rect pos;
  SDL_Surface *img;
  int i;

  for(i = 0; i < live_monster_count; i++) {
    if(move_monsters)
      live_monsters[i].monster->moveMonster(&live_monsters[i]);

    img =
      images[live_monsters[i].monster->
             image_index[getLiveMonsterFace(&live_monsters[i])]]->image;
    pos.x =
      live_monsters[i].pos_x * TILE_W - start_x + live_monsters[i].pixel_x;
    pos.y =
      live_monsters[i].pos_y * TILE_H - start_y + live_monsters[i].pixel_y;
    pos.w = img->w;
    pos.h = img->h;

    if(live_monsters[i].remove_me || !isOnScreen(&pos)) {
      removeLiveMonster(i);
    } else {
      live_monsters[i].monster->drawMonster(&pos, &live_monsters[i], surface,
                                            img);

      if(live_monsters[i].monster->breeds != NULL &&
         live_monsters[i].monster->max_children >
         live_monsters[i].child_count) {
        live_monsters[i].monster->breedMonster(&live_monsters[i], &pos);
      }
    }
  }
}

int
getLiveMonsterFace(LiveMonster * live)
{
  return live->face / live->monster->face_mod;
}

/**
   Return live monster if there's a one at position pos,
   NULL otherwise.
 */
LiveMonster *
detectMonster(Position * pos)
{
  int i;
  for(i = 0; i < live_monster_count; i++) {
    if(live_monsters[i].monster->detectMonster(pos, &live_monsters[i])) {
      return &live_monsters[i];
    }
  }
  return NULL;
}

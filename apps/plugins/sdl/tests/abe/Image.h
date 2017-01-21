#ifndef IMAGE_H
#define IMAGE_H

#include "Main.h"

#define IMAGES_DIR "images"

extern SDL_Surface *title, *score_image;
extern SDL_Surface *tom[13];

void loadImages();
void loadImagesFromTar();
void setAlphaBlends();

#define TYPE_WALL 1
#define TYPE_LADDER 2
#define TYPE_OBJECT 4
#define TYPE_DOOR 8
#define TYPE_HARMFUL 16
#define TYPE_SPRING 32
#define TYPE_SLIDE 64
#define TYPE_DECORATION 128

typedef struct image {
  char *name;
  SDL_Surface *image;
  Uint16 type;
  int monster_index;
} Image;

extern Image *images[256];
extern int image_count;

// known image indexes
extern int img_brick, img_rock, img_back, img_key, img_door, img_door2,
  img_key, img_smash, img_smash2, img_smash3, img_smash4;
extern int img_water, img_spring, img_spring2, img_spider, img_spider2,
  img_health;
extern int img_balloon[3], img_gem[3], img_bullet[4], img_slide_left[3],
  img_slide_right[3], img_slideback;

#endif

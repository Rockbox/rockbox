/**
   This file handles scanning the images directory and loading images
   into memory. It also keeps track of named images' indexes.
 */
#include "Image.h"

#define TAR_BLOCK_SIZE 512
#define TAR_NAME_SIZE 100
#define TAR_SIZE_SIZE 12
#define TAR_SIZE_OFFSET 124

SDL_Surface *title, *score_image;
SDL_Surface *tom[13];
Image *images[256];
int image_count;
int img_brick, img_rock, img_back, img_key, img_door, img_door2, img_key,
  img_smash, img_smash2, img_smash3, img_smash4;
int img_water, img_spring, img_spring2, img_spider, img_spider2, img_health;
int img_balloon[3], img_gem[3], img_bullet[4], img_slide_left[3],
  img_slide_right[3], img_slideback;
int alphacount = 0;
int alphas[256];

char *rb_strdup(const char *s)
{
    char *r = malloc(1+rb->strlen(s));
    rb->strlcpy(r,s, 999);
    return r;
}

/**
   Store the image in an array or a named img buffer.
 */
void
doLoadImage(char *filename, char *name)
{
  int type = TYPE_WALL;
  int monster = -1;
  SDL_Surface *image;

  fprintf(stderr, "\tLoading %s [%s]...\n", filename, name);
  fflush(stderr);

  image = SDL_LoadBMP(filename);
  if(image == NULL) {
    fprintf(stderr, "Couldn't load %s: %s\n", filename, SDL_GetError());
    fflush(stderr);
    return;
  }
  // set black as the transparent color key
  SDL_SetColorKey(image, SDL_SRCCOLORKEY,
                  SDL_MapRGBA(image->format, 0x00, 0x00, 0x00,
                              SDL_ALPHA_OPAQUE));

  // save the image
  if(!strcmp(name, "abe")) {
    title = image;
  } else if(!strcmp(name, "fonts")) {
    initFonts(image);
  } else if(!strcmp(name, "score")) {
    score_image = image;
  } else if(!strcmp(name, "tom1")) {
    tom[0] = image;
  } else if(!strcmp(name, "tom2")) {
    tom[1] = image;
  } else if(!strcmp(name, "tom3")) {
    tom[2] = image;
  } else if(!strcmp(name, "tom4")) {
    tom[3] = image;
  } else if(!strcmp(name, "tom5")) {
    tom[4] = image;
  } else if(!strcmp(name, "tom6")) {
    tom[5] = image;
  } else if(!strcmp(name, "tom7")) {
    tom[6] = image;
  } else if(!strcmp(name, "tom8")) {
    tom[7] = image;
  } else if(!strcmp(name, "jump1")) {
    tom[8] = image;
  } else if(!strcmp(name, "jump2")) {
    tom[9] = image;
  } else if(!strcmp(name, "climb1")) {
    tom[10] = image;
  } else if(!strcmp(name, "climb2")) {
    tom[11] = image;
  } else if(!strcmp(name, "climb3")) {
    tom[12] = image;
  } else {
    // a primitive hashtable
    if(!strcmp(name, "brick2")) {
      img_brick = image_count;
    } else if(!strcmp(name, "rock")) {
      img_rock = image_count;
    } else if(!strcmp(name, "back3d")) {
      img_back = image_count;
    } else if(!strcmp(name, "door")) {
      img_door = image_count;
      type = TYPE_DOOR;
    } else if(!strcmp(name, "door2")) {
      img_door2 = image_count;
      type = TYPE_DOOR;
    } else if(!strcmp(name, "key")) {
      type = TYPE_OBJECT;
      img_key = image_count;
    } else if(!strcmp(name, "health")) {
      type = TYPE_OBJECT;
      img_health = image_count;
    } else if(!strcmp(name, "balloon")) {
      type = TYPE_OBJECT;
      img_balloon[0] = image_count;
    } else if(!strcmp(name, "balloon2")) {
      type = TYPE_OBJECT;
      img_balloon[1] = image_count;
    } else if(!strcmp(name, "balloon3")) {
      type = TYPE_OBJECT;
      img_balloon[2] = image_count;
    } else if(!strcmp(name, "bullet1")) {
      img_bullet[0] = image_count;
      monster = MONSTER_BULLET;
    } else if(!strcmp(name, "bullet2")) {
      img_bullet[1] = image_count;
      monster = MONSTER_BULLET;
    } else if(!strcmp(name, "bullet3")) {
      img_bullet[2] = image_count;
      monster = MONSTER_BULLET;
    } else if(!strcmp(name, "bullet4")) {
      img_bullet[3] = image_count;
      monster = MONSTER_BULLET;
    } else if(!strcmp(name, "gem")) {
      type = TYPE_OBJECT;
      img_gem[0] = image_count;
    } else if(!strcmp(name, "gem2")) {
      type = TYPE_OBJECT;
      img_gem[1] = image_count;
    } else if(!strcmp(name, "gem3")) {
      type = TYPE_OBJECT;
      img_gem[2] = image_count;
    } else if(!strcmp(name, "endgame")) {
      monster = MONSTER_END_GAME;
    } else if(!strcmp(name, "wave")) {
      type = TYPE_HARMFUL;
      alphas[alphacount++] = image_count;
    } else if(!strcmp(name, "water")) {
      img_water = image_count;
      type = TYPE_HARMFUL;
      alphas[alphacount++] = image_count;
    } else if(!strcmp(name, "ladder")) {
      type = TYPE_LADDER;
    } else if(!strcmp(name, "spring")) {
      type = TYPE_SPRING;
      img_spring = image_count;
    } else if(!strcmp(name, "spring2")) {
      type = TYPE_SPRING;
      img_spring2 = image_count;
    } else if(!strcmp(name, "crab1") || !strcmp(name, "crab2")) {
      monster = MONSTER_CRAB;
    } else if(!strcmp(name, "ghost") || !strcmp(name, "ghost2")) {
      monster = MONSTER_GHOST;
      alphas[alphacount++] = image_count;
    } else if(!strcmp(name, "smash")) {
      monster = MONSTER_SMASHER;
      img_smash = image_count;
    } else if(!strcmp(name, "smash2")) {
      img_smash2 = image_count;
    } else if(!strcmp(name, "smash3")) {
      monster = MONSTER_SMASHER2;
      img_smash3 = image_count;
    } else if(!strcmp(name, "smash4")) {
      img_smash4 = image_count;
    } else if(!strcmp(name, "demon") || !strcmp(name, "demon2")) {
      monster = MONSTER_DEMON;
    } else if(!strcmp(name, "platform")) {
      monster = MONSTER_PLATFORM;
    } else if(!strcmp(name, "platform2")) {
      monster = MONSTER_PLATFORM2;
    } else if(!strcmp(name, "spider")) {
      monster = MONSTER_SPIDER;
      img_spider = image_count;
    } else if(!strcmp(name, "spider2")) {
      img_spider2 = image_count;
    } else if(!strcmp(name, "bear1")) {
      monster = MONSTER_BEAR;
    } else if(!strcmp(name, "bear2")) {
      monster = MONSTER_BEAR;
    } else if(!strcmp(name, "bear3")) {
      monster = MONSTER_BEAR;
    } else if(!strcmp(name, "bear4")) {
      monster = MONSTER_BEAR;
    } else if(!strcmp(name, "bear5")) {
      monster = MONSTER_BEAR;
    } else if(!strcmp(name, "bear6")) {
      monster = MONSTER_BEAR;
    } else if(!strcmp(name, "cannon")) {
      monster = MONSTER_CANNON;
    } else if(!strcmp(name, "cannon2")) {
      monster = MONSTER_CANNON2;
    } else if(!strcmp(name, "torch1") || !strcmp(name, "torch2")
              || !strcmp(name, "torch3")) {
      monster = MONSTER_TORCH;
    } else if(!strcmp(name, "star1") || !strcmp(name, "star2")
              || !strcmp(name, "star3") || !strcmp(name, "star4")) {
      monster = MONSTER_STAR;
    } else if(!strcmp(name, "arrow1") || !strcmp(name, "arrow2")) {
      monster = MONSTER_ARROW;
    } else if(!strcmp(name, "fire1") || !strcmp(name, "fire2")
              || !strcmp(name, "fire3")) {
      monster = MONSTER_FIRE;
    } else if(!strcmp(name, "bat1") || !strcmp(name, "bat2")) {
      monster = MONSTER_BAT;
    } else if(!strcmp(name, "slide1")) {
      type = TYPE_SLIDE;
      img_slide_right[0] = image_count;
    } else if(!strcmp(name, "slide2")) {
      type = TYPE_SLIDE;
      img_slide_right[1] = image_count;
    } else if(!strcmp(name, "slide3")) {
      type = TYPE_SLIDE;
      img_slide_right[2] = image_count;
    } else if(!strcmp(name, "slide4")) {
      type = TYPE_SLIDE;
      img_slide_left[0] = image_count;
    } else if(!strcmp(name, "slide5")) {
      type = TYPE_SLIDE;
      img_slide_left[1] = image_count;
    } else if(!strcmp(name, "slide6")) {
      type = TYPE_SLIDE;
      img_slide_left[2] = image_count;
    } else if(!strcmp(name, "slideback")) {
      img_slideback = image_count;
    } else if(!strcmp(name, "crystal")) {
      type = TYPE_DECORATION;
    }
    // store the image
    if(!(images[image_count] = (Image *) malloc(sizeof(Image)))) {
      fprintf(stderr, "Out of memory.");
      fflush(stderr);
      exit(0);
    }
    images[image_count]->image = image;
    images[image_count]->name = rb_strdup(name);
    images[image_count]->type = type;

    // create a monster if needed
    if(monster > -1) {
      addMonsterImage(monster, image_count);
    }
    images[image_count]->monster_index = monster;

    showLoadingProgress();
    image_count++;
  }
}

int
selectDirEntry(const struct dirent *d)
{
    //return (strstr(d->d_name, ".bmp") || strstr(d->d_name, ".BMP") ? 1 : 0);
    return 0;
}

char *
getImageName(char *s)
{
  char *r = rb_strdup(s);
  *(r + rb->strlen(r) - 4) = 0;
  return r;
}

/**
   Load every bmp file from the ./images directory.
 */
void
loadImages()
{
  loadImagesFromTar();
  setAlphaBlends();

  /*
     image_count = 0;

     fprintf(stderr, "Looking for images in %s \n", xstr(BASE_DIR) PATH_SEP IMAGES_DIR PATH_SEP);
     fflush(stderr);

     // it's important to always load the images in the same order.
     struct dirent **namelist;
     int n;
     n = scandir(xstr(BASE_DIR) PATH_SEP IMAGES_DIR PATH_SEP, &namelist, selectDirEntry, alphasort);
     if(n < 0) {
     fprintf(stderr, "Can't sort directory: %s\n", xstr(BASE_DIR) PATH_SEP IMAGES_DIR PATH_SEP);
     fflush(stderr);
     exit(0);
     } else {
     char *name;
     char path[PATH_SIZE];
     while(n--) {
     name = getImageName(namelist[n]->d_name);
     sprintf(path, xstr(BASE_DIR) PATH_SEP IMAGES_DIR PATH_SEP "%s", namelist[n]->d_name);
     doLoadImage(path, name);
     free(name);
     free(namelist[n]);
     }
     free(namelist);
     }
   */
}

/**
   Loading from the tar has several benefits:
   -only 1 file to deal with
   -keeps the order of images constant
   -allows me to append new images to the tar while the 
    order of the existing ones doesn't change.
*/
void
loadImagesFromTar()
{
    char tmp_path[PATH_SIZE];
    FILE *tmp = NULL, *fp;
    char path[PATH_SIZE];
    char buff[TAR_BLOCK_SIZE];    // a tar block
    int end = 0;
    int i;
    int mode = 0;                 // 0-header, 1-file
    char name[TAR_NAME_SIZE + 1], size[TAR_SIZE_SIZE + 1];
    long filesize = 0;
    int found;
    int blocks_read = 0;
    int block = 0;

    image_count = 0;
    mkshuae();
    rb->snprintf(tmp_path, 999, "%s%s", getHomeUserAbe(), "tmp.bmp");

    rb->snprintf(path, 999, xstr(BASE_DIR) PATH_SEP IMAGES_DIR PATH_SEP "images.tar");
    //rb->splashf(HZ, "Opening %s for reading.\n", path);
    fflush(stderr);
    fp = fopen(path, "rb");
    if(!fp) {
        fprintf(stderr, "Can't open tar file.\n");
        fflush(stderr);
        exit(-1);
    }
    while(1)
    {
        if(fread(buff, 1, TAR_BLOCK_SIZE, fp) < TAR_BLOCK_SIZE)
            break;                    // EOF or error
        if(!mode)
        {
            // are we at the end?
            found = 0;
            for(i = 0; i < TAR_BLOCK_SIZE; i++)
            {
                if(buff[i])
                {
                    found = 1;
                    break;
                }
            }
            if(!found)
            {
                end++;
                // a tar file ends with 2 NULL blocks
                if(end >= 2)
                    break;
            }
            else
            {
                // Get the name
                memcpy(name, buff, TAR_NAME_SIZE);
                // add a NUL if needed
                found = 0;
                for(i = 0; i < TAR_NAME_SIZE; i++)
                {
                    if(!name[i])
                    {
                        found = 1;
                        break;
                    }
                }
                if(!found)
                    name[TAR_NAME_SIZE] = '\0';
                // Remove the .tmp
                *(name + rb->strlen(name) - 4) = 0;
                // Get the size
                memcpy(size, buff + TAR_SIZE_OFFSET, TAR_SIZE_SIZE);
                size[TAR_SIZE_SIZE] = '\0';
                filesize = strtol(size, NULL, 8);
                blocks_read =
                    filesize / TAR_BLOCK_SIZE + (filesize % TAR_BLOCK_SIZE ? 1 : 0);
                fprintf(stderr, "Found: >%s< size=>%ld< blocks=>%d<\n", name,
                        filesize, blocks_read);
                fflush(stderr);
                mode = 1;

                // open a temp file to extract this image to
                if(!(tmp = fopen(tmp_path, "wb"))) {
                    //rb->splashf(HZ, "Cannot open temp file for writing: %s\n",
                    //            tmp_path);
                    exit(0);
                }
            }
        }
        else
        {
            blocks_read--;

            // write to temp file
            fwrite(buff,
                   (!blocks_read ? filesize % TAR_BLOCK_SIZE : TAR_BLOCK_SIZE), 1,
                   tmp);

            if(!blocks_read)
            {
                mode = 0;
                fclose(tmp);

                // load the image
                doLoadImage(tmp_path, name);
            }
        }
        block++;
    }
    fclose(fp);

    // remove the tmp file
    rb->remove(tmp_path);
}

void
setAlphaBlends()
{
  int i;
  for(i = 0; i < alphacount; i++) {
    if(mainstruct.alphaBlend) {
      SDL_SetAlpha(images[alphas[i]]->image, SDL_RLEACCEL | SDL_SRCALPHA,
                   128);
    } else {
      SDL_SetAlpha(images[alphas[i]]->image, 0, 0);
    }
  }
}

#include "Util.h"

// special effects
#define NO_EFFECT 0
#define SHIMMER_EFFECT 1
#define DAMAGE_EFFECT 2
int effect = NO_EFFECT;
SDL_Rect fx_rect;
SDL_Surface *fx_surface;
int fx_x, fx_y;

int
getSectionDiff(int a1, int a2, int b1, int b2)
{
  if(a1 <= b1 && a2 > b1) {
    return a2 - b1;
  } else if(a1 >= b1 && a1 < b2) {
    return b2 - a1;
  } else {
    return 0;
  }
}

int
contains(SDL_Rect * a, int x, int y)
{
  return (x >= a->x && x < a->x + a->w &&
          y >= a->y && y < a->y + a->h ? 1 : 0);
}

int
sectionIntersects(int a1, int a2, int b1, int b2)
{
  return ((a1 <= b1 && a2 > b1) || (a1 >= b1 && a1 < b2));
}

int
intersects(SDL_Rect * a, SDL_Rect * b)
{
  return (sectionIntersects(a->x, a->x + a->w, b->x, b->x + b->w) &&
          sectionIntersects(a->y, a->y + a->h, b->y, b->y + b->h));
}

void
writeEndianData(SDL_RWops * rwop, Uint16 * block, size_t size)
{
  size_t n;
  for(n = 0; n < size; n++) {
    SDL_WriteLE16(rwop, *(block + n));
  }
}

int
intersectsBy(SDL_Rect * a, SDL_Rect * b, int value)
{
  int w, h;
  w = getSectionDiff(a->x, a->x + a->w, b->x, b->x + b->w);
  h = getSectionDiff(a->y, a->y + a->h, b->y, b->y + b->h);
  return (w > value && h > value ? (w > h ? w : h) : 0);
}

/**
   Simple run-length compression.
   For n entries of the same value, if n > 3 then write (each entry an int):
   key n value
   (Key should not appear as a valid image_index, currently key is f0f0f0f0.)
   return 0 on error, number of entries written on success
*/
int
compress(Uint16 * buff, size_t size, SDL_RWops * rwop)
{
  size_t size_written = 0;
#ifdef USE_COMPRESSION
  //  printf("Compressing...\n");
  Uint16 *block;
  size_t i, t = 0;
  Uint16 block_note = BLOCK_NOTE;
  if(!(block = (Uint16 *) malloc(sizeof(Uint16) * size))) {
    printf("Out of memory when writing compressed file");
    fflush(stderr);
    exit(-1);
  }
  for(i = 0; i < size; i++) {
    if(i > 0 && buff[i] != block[t - 1]) {
      //    printf("\tdiff. int found i=%ld t=%ld buff[i]=%ld block[t - 1]=%ld - ", i, t, buff[i], block[t - 1]);
      if(t > 3) {
        //    printf("\tcompressed block\n");
        // compressed write
        SDL_WriteLE16(rwop, block_note);
        SDL_WriteLE16(rwop, t);
        SDL_WriteLE16(rwop, *block);

        size_written += 3;
      } else {
        //    printf("\tnormal block\n");
        // normal write
        writeEndianData(rwop, block, t);

        size_written += t;
      }
      t = 0;
    }
    block[t++] = buff[i];
  }
  // write the last block
  //  printf("\tlast diff. int found i=%ld t=%ld buff[i]=%ld block[t - 1]=%ld - ", i, t, buff[i], block[t - 1]);
  if(t > 3) {
    //  printf("\tcompressed block\n");
    // compressed write
    SDL_WriteLE16(rwop, block_note);
    SDL_WriteLE16(rwop, t);
    SDL_WriteLE16(rwop, *block);

    size_written += 3;
  } else {
    //  printf("\tnormal block\n");
    // normal write
    writeEndianData(rwop, block, t);

    size_written += t;
  }
  free(block);
#else
  //  printf("Writing uncompressed...\n");
  for(n = 0; n < size; n++) {
    SDL_WriteLE16(rwop, *(buff + n));
  }
  size_written += size;
#endif
  return size_written;
}

/**
   Read above compression and decompress into buff.
   return number of items read into buff (should equal size on success)
*/
int
decompress(Uint16 * buff, size_t size, SDL_RWops * rwop)
{
  Uint16 *block;
  size_t real_size;
  size_t i = 0, t = 0, r = 0, start = 0, n = 0;
  size_t count;
  Uint16 value;

  // read the file
  if(!(block = (Uint16 *) malloc(sizeof(Uint16) * size))) {
    printf("Out of memory when writing compressed file");
    fflush(stderr);
    exit(-1);
  }
  //  real_size = fread(block, sizeof(Uint16), size, fp);
  real_size = SDL_RWread(rwop, block, sizeof(Uint16), size);
  if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
    for(n = 0; n < real_size; n++) {
      *(block + n) = SDL_SwapLE16(*(block + n));
    }
  }
#ifdef USE_COMPRESSION
  printf("Decompressing... real_size=%ld\n", (long int) real_size);
  for(t = 0; t < real_size;) {

    if((Uint16) block[t] == BLOCK_NOTE) {
      // write the previous block
      if(start != t) {
        count = t - start;
        //    printf("\tuncompressed block: i=%ld start=%ld t=%d count=%ld\n", i, start, t, count);
        memcpy(buff + i, block + start, count * sizeof(Uint16));
        i += count;
      }
      // write the compressed block
      // (these are for readability.)
      count = block[t + 1];
      value = block[t + 2];
      //    printf("\tcompressed block start: i=%ld start=%ld t=%ld count=%ld\n", i, start, t, count);
      for(r = 0; r < count; r++) {
        buff[i++] = value;
      }
      t += 3;
      start = t;
      //    printf("\tcompressed block end: i=%ld start=%ld t=%ld count=%ld\n", i, start, t, count);
    } else {
      t++;
    }
  }
  // write the last block
  if(start != t) {
    count = t - start;
    //  printf("\tuncompressed block: i=%ld start=%ld t=%d count=%ld\n", i, start, t, count);
    memcpy(buff + i, block + start, count * sizeof(Uint16));
    i += count;
  }
  //  printf("\tdone: i=%ld start=%ld t=%d count=%ld\n", i, start, t, count);
  free(block);
  return i;
#else
  //  printf("Read uncompressed.");
  memcpy(buff, block, real_size * sizeof(Uint16));
  free(block);
  return real_size * sizeof(Uint16);
#endif
}

void
createBack(SDL_Surface ** back_surface)
{
  int x, y;
  SDL_Rect pos;
  SDL_Surface *img = images[img_back]->image;

  if(!(*back_surface = SDL_CreateRGBSurface(SDL_HWSURFACE,
                                            screen->w + img->w,
                                            screen->h + img->h,
                                            screen->format->BitsPerPixel,
                                            0, 0, 0, 0))) {
    printf("Error creating surm_face: %s\n", SDL_GetError());
    fflush(stderr);
    return;
  }

  /*
     for(y = 0; y < screen->h / TILE_H; y+=img->w/TILE_H) {
     for(x = 0; x < screen->w / TILE_W; x+=img->h/TILE_W) {
     pos.x = x * TILE_W;
     pos.y = y * TILE_H;
     pos.w = img->w;
     pos.h = img->h;
     SDL_BlitSurface(img, NULL, *back_surface, &pos);
     }
     }
   */
  for(y = 0; y < (*back_surface)->h; y += img->h) {
    for(x = 0; x < (*back_surface)->w; x += img->w) {
      pos.x = x;
      pos.y = y;
      pos.w = img->w;
      pos.h = img->h;
      SDL_BlitSurface(img, NULL, *back_surface, &pos);
    }
  }
}

void
shimmerEffect(SDL_Rect * rect, SDL_Surface * surface)
{
  effect = SHIMMER_EFFECT;
  memcpy(&fx_rect, rect, sizeof(fx_rect));
  fx_surface = surface;
  fx_y = rect->h;
}

void
damageEffect(SDL_Rect * rect, SDL_Surface * surface)
{
  effect = DAMAGE_EFFECT;
  memcpy(&fx_rect, rect, sizeof(fx_rect));
  fx_surface = surface;
  fx_y = rect->h;
}

void
processShimmer()
{
  int row[3][100], size[3][100];
  Uint32 color[3][100];
  int count;
  int x, j, r;
  SDL_Rect pos;
  int done;

  for(r = 0; r < 3; r++) {
    if(fx_y < 0) {
      effect = NO_EFFECT;
      break;
    }
    count = 0;
    for(x = 0; x < fx_rect.w; x += 2) {
      j = (int) (7.0 * rand() / (RAND_MAX));
      if(j == 1) {
        row[r][count] = x;
        size[r][count] = (int) (5.0 * rand() / (RAND_MAX)) + 2;
        if(effect == SHIMMER_EFFECT) {
          color[r][count] = SDL_MapRGBA(fx_surface->format,
                                        128 +
                                        (int) (100.0 * rand() / (RAND_MAX)),
                                        128 +
                                        (int) (100.0 * rand() / (RAND_MAX)),
                                        128 +
                                        (int) (100.0 * rand() / (RAND_MAX)),
                                        0x00);
        } else {
          if((int) (2.0 * rand() / (RAND_MAX)) == 1) {
            color[r][count] = SDL_MapRGBA(fx_surface->format,
                                          255,
                                          40 +
                                          (int) (50.0 * rand() / (RAND_MAX)),
                                          40 +
                                          (int) (50.0 * rand() / (RAND_MAX)),
                                          0x00);
          } else {
            color[r][count] = SDL_MapRGBA(fx_surface->format,
                                          255,
                                          255,
                                          40 +
                                          (int) (50.0 * rand() / (RAND_MAX)),
                                          0x00);
          }
        }
        count++;
      }
    }
    done = 0;
    while(!done) {
      done = 1;
      for(x = 0; x < count; x++) {
        if(size[r][x] > 0) {
          pos.x = fx_rect.x + row[r][x] - (size[r][x] / 2);
          if(effect == SHIMMER_EFFECT)
            pos.y = fx_rect.y + fx_rect.h - (fx_y - (size[r][x] / 2));
          else
            pos.y = fx_rect.y + fx_y - (size[r][x] / 2);
          pos.w = size[r][x];
          pos.h = size[r][x];
          SDL_FillRect(fx_surface, &pos, color[r][x]);
          if(--size[r][x] > 0)
            done = 0;
        }
      }
    }
    fx_y -= 5;
  }
  SDL_UpdateRects(fx_surface, 1, &fx_rect);
  //SDL_Flip(surface);
  //SDL_Delay(10);
}

void
processEffects()
{
  switch (effect) {
  case SHIMMER_EFFECT:
  case DAMAGE_EFFECT:
    processShimmer();
    break;
  }
}

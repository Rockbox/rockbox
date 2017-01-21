#include <errno.h>
#include "MapIO.h"

void
saveMapPath(char *path)
{
  FILE *fp;
  size_t new_size, written;
  Uint16 *compressed_map;
  char *err;
  SDL_RWops *rwop;

  printf("Saving map %s\n", path);
  fflush(stdout);

  if(!(fp = fopen(path, "wb"))) {
    printf("Can't open file for writing\n");
    fflush(stderr);
    return;
  }

  rwop = SDL_RWFromFP(fp, 1);

  // write the header
  SDL_WriteLE16(rwop, map.w);
  SDL_WriteLE16(rwop, map.h);

  // compression step 1
  printf("Compressing...\n");
  compressed_map = compressMap(&new_size);
  printf("Compressed map. old_size=%ld new_size=%ld\n",
          (long int) (LEVEL_COUNT * map.w * map.h), (long int) new_size);
  fflush(stderr);
  // write out and further compress in step 2
  written = compress(compressed_map, new_size, rwop);
  printf(
          "Compressed map step2. Written %ld ints. Compression ration: %f.2%%\n",
          (long int) written,
          (float) written / ((float) (LEVEL_COUNT * map.w * map.h) / 100.0));
  fflush(stderr);

  // Does this close the file? It should according to the constructor...
  SDL_RWclose(rwop);
  free(compressed_map);
}

void
saveMap()
{
  char path[PATH_SIZE];
  sprintf(path, xstr(BASE_DIR) PATH_SEP MAPS_DIR PATH_SEP "%s.dat", map.name);
  saveMapPath(path);
}

// call these after initMap()!
int
loadMap(int draw_map)
{
  char path[PATH_SIZE];
  sprintf(path, xstr(BASE_DIR) PATH_SEP MAPS_DIR PATH_SEP "%s.dat", map.name);
  return loadMapPath(path, draw_map);
}

int
loadMapPath(char *path, int draw_map)
{
  FILE *fp;
  size_t size;
  Uint16 *read_buff;
  int count_read;
  char *err;
  SDL_RWops *rwop;
  int x, y, i;

  printf("Loading map %s\n", path);
  fflush(stdout);
  if(!(fp = fopen(path, "rb"))) {
    printf("Can't open file for reading\n");
    fflush(stderr);
    return 0;
  }
  rwop = SDL_RWFromFP(fp, 1);

  // read the header
  map.w = SDL_ReadLE16(rwop);
  map.h = SDL_ReadLE16(rwop);

  printf("map dimensions %dx%d\n", map.w, map.h);
  fflush(stdout);

  // compression step 1: read compressed data from disk
  // FIXME: what would be nicer is to only allocate as much mem as used on disk.
  size = LEVEL_COUNT * map.w * map.h;
  printf("size %u\n", size);
  fflush(stdout);
  if(!(read_buff = (Uint16 *) malloc(sizeof(Uint16) * size))) {
    printf("Out of memory on map read.");
    fflush(stderr);
    exit(0);
  }
  count_read = decompress(read_buff, size, rwop);
  printf("read %d ints\n", count_read);
  fflush(stderr);
  // Does this close the file? It should according to the constructor...
  SDL_RWclose(rwop);

  // step 2: further uncompress
  decompressMap(read_buff);
  free(read_buff);

  // clean map... delete this...
  printf("*** Debug code: removing stuff in LEVEL_FORE.\n");
  for(y = 0; y < map.h; y++) {
    for(x = 0; x < map.w; x++) {
      i = x + (y * map.w);
      if(map.image_index[LEVEL_FORE][i] == 0)
        map.image_index[LEVEL_FORE][i] = EMPTY_MAP;
    }
  }

  resetCursor();
  if(draw_map)
    drawMap();
  return 1;
}










int
convertMap(char *from, char *to)
{
  FILE *fp;
  int *buff;
  int count_read;
  char *err;
  int w, h, i;

  printf("Reading map to be converted %s\n", from);
  fflush(stdout);
  if(!(fp = fopen(from, "rb"))) {
    err = "whatever";
    printf("Can't open file for reading: %s\n", err);
    fflush(stderr);
    free(err);
    return 0;
  }
  // read the header
  fread(&(w), sizeof(w), 1, fp);
  fread(&(h), sizeof(h), 1, fp);

  if((buff = (int *) malloc(sizeof(int) * w * h)) == NULL) {
    printf("Can't allocate memory!\n");
    fflush(stderr);
    return 0;
  }
  count_read = fread(buff, sizeof(int), w * h, fp);
  printf("read %d ints\n", count_read);
  fflush(stderr);
  fclose(fp);

  // now throw away the second WORD and resave
  printf("Saving converting map %s\n", to);
  fflush(stdout);
  if(!(fp = fopen(to, "wb"))) {
    err = "whatever";
    printf("Can't open file for writing: %s\n", err);
    fflush(stderr);
    free(err);
    return 0;
  }
  // read the header
  fwrite(&w, sizeof(Uint16), 1, fp);
  fwrite(&h, sizeof(Uint16), 1, fp);
  for(i = 0; i < count_read; i++) {
    fwrite(buff + i, sizeof(Uint16), 1, fp);
  }
  fclose(fp);
  free(buff);
  return 1;
}

/** Remove unnecesary EMPTY_MAPs. For example a 4 tile wide stone becomes a 1 int number.
	return new number of elements in new_size. (so num of bytes=new_size * sizeof(int)).
	caller must free returned pointer.
	call this method before calling Utils.compress(). This prepares the map
	for better compression by understanding the its structure. This doesn't 
	compress the map that much, but combined with Utils.compress() map files
	can go from 12M to 14K!
*/
Uint16 *
compressMap(size_t * new_size)
{
  Uint16 *q;
  Uint16 n;
  int level, i, x, y;
  size_t t = 0;

  if(!(q = (Uint16 *) malloc(sizeof(Uint16) * map.w * map.h * LEVEL_COUNT))) {
    printf("Out of memory in compressMap.");
    fflush(stderr);
    exit(-1);
  }
  for(level = 0; level < LEVEL_COUNT; level++) {
    for(y = 0; y < map.h; y++) {
      for(x = 0; x < map.w;) {
        i = x + (y * map.w);
        n = map.image_index[level][i];
        *(q + t) = n;
        t++;
        if(n != EMPTY_MAP) {
          // skip ahead
          x += images[n]->image->w / TILE_W;
        } else {
          x++;
        }
      }
    }
  }
  *new_size = t;
  return q;
}

/**
   Decompress map by adding back missing -1-s. See compressMap() for
   details.
 */
void
decompressMap(Uint16 * p)
{
  int level, i, x, y, r;
  Uint16 n;
  size_t t = 0;

  for(level = 0; level < LEVEL_COUNT; level++) {
    for(y = 0; y < map.h; y++) {
      for(x = 0; x < map.w;) {
        n = *(p + t);
        t++;
        i = x + (y * map.w);
        map.image_index[level][i] = n;
        x++;
        if(n != EMPTY_MAP) {
          for(r = 1; r < images[n]->image->w / TILE_W && x < map.w; r++, x++) {
            map.image_index[level][i + r] = EMPTY_MAP;
          }
        }
      }
    }
  }
}

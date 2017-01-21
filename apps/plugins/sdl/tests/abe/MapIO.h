#ifndef MAPIO_H
#define MAPIO_H

#include "Main.h"
#include "SDL_rwops.h"
#include "SDL_endian.h"

#include "Directories.h"

void saveMap();
int loadMap(int drawMap);
void saveMapPath(char *path);
int loadMapPath(char *path, int drawMap);

/** Remove unnecesary EMPTY_MAPs. For example a 4 tile wide stone becomes a 1 int number.
	return new number of elements in new_size. (so num of bytes=new_size * sizeof(int)).
	caller must free returned pointer.
	call this method before calling Utils.compress(). This prepares the map
	for better compression by understanding the its structure. This doesn't 
	compress the map that much, but combined with Utils.compress() map files
	can go from 12M to 14K!
*/
Uint16 *compressMap(size_t * new_size);
void decompressMap(Uint16 * data);
int convertMap(char *from, char *to);

#endif

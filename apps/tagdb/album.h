#ifndef __ALBUM_H__
#define __ALBUM_H__

#include "config.h"
#include <stdio.h>

struct album_entry {
	char* name;		// album name
	char* key;		// key for sorting/searching: album___artist___directory
	uint32_t artist;	// pointer to artist
	uint32_t *song;		// song-pointers
	struct album_size {
		uint32_t name_len;	// length of this field (must be mulitple of 4)
		uint32_t song_count;	// number of song pointers
	} size;	// keeps the size of this thing
	unsigned char flag;		// flags
};

struct album_entry* new_album_entry(const uint32_t name_len, const uint32_t song_count);
/* Creates a new album_entry with the specified sizes
 * Returns a pointer to the structure on success,
 * NULL when malloc() fails
 */

int album_entry_destruct(struct album_entry *e);
/* Destructs the given album_entry and free()'s it's memory
 * returns ERR_NONE on success (can never fail)
 */

inline int album_entry_resize(struct album_entry *e, const uint32_t name_len, const uint32_t song_count);
/* Change the size of the entry
 * returns ERR_NONE on succes
 *         ERR_MALLOC when malloc() fails
 */

int album_entry_serialize(FILE *fd, const struct album_entry *e);
/* Serializes the entry in the file at the current position
 * returns ERR_NONE on success
 *         ERR_FILE on fwrite() failure
 */

int album_entry_unserialize(struct album_entry* *e, FILE *fd);
/* Unserializes an entry from file into a new structure
 * The address of the structure is saved into *e
 * returns ERR_NONE on success
 *         ERR_MALLOC on malloc() failure
 *         ERR_FILE on fread() failure
 */

int album_entry_write(FILE *fd, struct album_entry *e, struct album_size *s);
/* Writes the entry to file in the final form
 * returns ERR_NONE on success
 *         ERR_FILE on fwrite() failure
 *         ERR_MALLOC when e could not be resized due to malloc() problems
 * If s is smaller than e, s is used!!!
 */

inline int album_entry_compare(const struct album_entry *a, const struct album_entry *b);
/* Compares 2 entries
 * When a < b it returns <0
 *      a = b             0
 *      a > b            >0
 */

struct album_size* new_album_size();
/* Creates a new size structure
 * returns a pointer to the structure on success,
 * NULL on malloc() failure
 */

inline uint32_t album_size_get_length(const struct album_size *size);
/* Calculates the length of the entry when written by album_entry_write()
 * returns the length on success (can never fail)
 */

inline int album_size_max(struct album_size *s, const struct album_entry *e);
/* Updates the album_size structure to contain the maximal lengths of either
 * the original entry in s, or the entry e
 * returns ERR_NONE on success (can never fail)
 */

int album_size_destruct(struct album_size *s);
/* destructs the album_size structure
 * returns ERR_NONE on success (can never fail)
 */


int album_entry_add_song_mem(struct album_entry *e, struct album_size *s, const uint32_t song);
/* Adds the song to the array
 * returns ERR_NONE on success
 *         ERR_MALLOC on malloc() failure
 */

int album_entry_add_song_file(FILE *fd, struct album_entry *e, struct album_size *s, const uint32_t song);
/* Adds the song to the serialized entry in the file
 * When this fails, the entry is invalidated and the function returns
 * ERR_NO_INPLACE_UPDATE
 * returns ERR_NONE on success
 *         ERR_NO_INPLACE_UPDATE (see above)
 *         ERR_FILE on fwrite() failure
 */

#endif

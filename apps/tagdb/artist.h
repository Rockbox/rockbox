#ifndef __ARTIST_H__
#define __ARTIST_H__

#include "config.h"
#include <stdio.h>
#include <stdint.h>

struct artist_entry {
	char* name;		// artist name
	uint32_t *album;	// album-pointers
	struct artist_size {
		uint32_t name_len;	// length of this field (must be mulitple of 4)
		uint32_t album_count;	// number of album pointers
	} size;
	unsigned char flag;		// flags
};

struct artist_entry* new_artist_entry(const uint32_t name_len, const uint32_t album_count);
/* Creates a new artist_entry with the specified sizes
 * Returns a pointer to the structure on success,
 * NULL on failure
*/

int artist_entry_destruct(struct artist_entry *e);
/* Destructs the given artist_entry and free()'s it's memory
 * returns ERR_NONE on success (can't fail)
 */

int artist_entry_resize(struct artist_entry *e, const uint32_t name_len, const uint32_t album_count);
/* Change the size of the entry
 * returns ERR_NONE on succes
 *         ERR_MALLOC on malloc() failure
 */

int artist_entry_serialize(FILE *fd, const struct artist_entry *e);
/* Serializes the entry in the file at the current position
 * returns ERR_NONE on success
 *         ERR_FILE on fwrite() failure
 */

int artist_entry_unserialize(struct artist_entry* *e, FILE *fd);
/* Unserializes an entry from file into a new structure
 * The address of the structure is saved into *e
 * returns ERR_NONE on success
 *         ERR_MALLOC on malloc() failure
 *         ERR_FILE on fread() failure
 */

int artist_entry_write(FILE *fd, const struct artist_entry *e, const struct artist_size *s);
/* Writes the entry to file in the final form
 * returns ERR_NONE on success
 *         ERR_FILE on fwrite() failure
 */

inline int artist_entry_compare(const struct artist_entry *a, const struct artist_entry *b);
/* Compares 2 entries
 * When a < b it returns <0
 *      a = b             0
 *      a > b            >0
 */

struct artist_size* new_artist_size();
/* Creates a new size structure
 * returns a pointer to the structure on success,
 * NULL on failure
 */

inline uint32_t artist_size_get_length(const struct artist_size *size);
/* Calculates the length of the entry when written by artist_entry_write()
 * returns the length on success (can't fail)
 */

inline int artist_size_max(struct artist_size *s, const struct artist_entry *e);
/* Updates the artist_size structure to contain the maximal lengths of either
 * the original entry in s, or the entry e
 * returns ERR_NONE on success (can't fail)
 */

int artist_size_destruct(struct artist_size *s);
/* destructs the artist_size structure
 * returns ERR_NONE on success (can't fail)
 */


int artist_entry_add_album_mem(struct artist_entry *e, struct artist_size *s, const uint32_t album);
/* Adds the album to the array
 * returns ERR_NONE on success
 *         ERR_MALLOC on malloc() failure
 */

int artist_entry_add_album_file(FILE *fd, struct artist_entry *e, struct artist_size *s, const uint32_t album);
/* Adds the album to the serialized entry in the file
 * When this fails, the entry is invalidated and the function returns
 * ERR_NO_INPLACE_UPDATE
 * returns ERR_NONE on success
 *         ERR_NO_INPLACE_UPDATE (see above)
 *         ERR_FILE on fread()/fwrite() error
 */

#endif

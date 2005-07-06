#ifndef __SONG_H__
#define __SONG_H__

#include "config.h"
#include <stdio.h>
#include <stdint.h>

struct song_entry {
	char* name;		// song name
	
	uint32_t artist;	// pointer to artist
	uint32_t album;		// pointer to album
	uint32_t file;		// pointer to file
	
	char* genre;		// genre

	uint16_t bitrate;	// bitrate (-1 = VBR or unknown)
	uint16_t year;
	uint32_t playtime;	// in seconds
	uint16_t track;
	uint16_t samplerate;	// in Hz

	struct song_size {
		uint32_t name_len;	// must be mulitple of 4
		uint32_t genre_len;	// must be multiple of 4
	} size;
	unsigned char flag;	// flags
};

struct song_entry* new_song_entry(const uint32_t name_len, const uint32_t genre_len);
/* Creates a new song_entry with the specified sizes
 * Returns a pointer to the structure on success,
 * NULL on failure
 */

int song_entry_destruct(struct song_entry *e);
/* Destructs the given song_entry and free()'s it's memory
 * returns 0 on success, 1 on failure
 */

inline int song_entry_resize(struct song_entry *e, const uint32_t name_len, const uint32_t genre_len);
/* Change the size of the entry
 * returns 0 on succes, 1 on failure
 */

int song_entry_serialize(FILE *fd, const struct song_entry *e);
/* Serializes the entry in the file at the current position
 * returns 0 on success, 1 on failure
 */

int song_entry_unserialize(struct song_entry* *e, FILE *fd);
/* Unserializes an entry from file into a new structure
 * The address of the structure is saved into *e
 * returns 0 on success
 *         1 on malloc() failure
 *         2 on fread() failure
 */

int song_entry_write(FILE *fd, struct song_entry *e, struct song_size *s);
/* Writes the entry to file in the final form
 * returns 0 (0) on success, 1 (1) on failure
 */

inline int song_entry_compare(const struct song_entry *a, const struct song_entry *b);
/* Compares 2 entries
 * When a < b it returns <0
 *      a = b             0
 *      a > b            >0
 */

struct song_size* new_song_size();
/* Creates a new size structure
 * returns a pointer to the structure on success,
 * NULL on failure
 */

inline uint32_t song_size_get_length(const struct song_size *size);
/* Calculates the length of the entry when written by song_entry_write()
 * returns the length on success, 0xffffffff on failure
 */

inline int song_size_max(struct song_size *s, const struct song_entry *e);
/* Updates the song_size structure to contain the maximal lengths of either
 * the original entry in s, or the entry e
 * returns 0 on success, 1 on failure
 */

int song_size_destruct(struct song_size *s);
/* destructs the song_size structure
 * returns 0 on success, 1 on failure
 */

#endif

#ifndef __HEADER_H__
#define __HEADER_H__

#include "config.h"

#define HEADER_SIZE 68

struct header {
	char magic[3];			// (four bytes: 'R' 'D' 'B' and a byte for version. This is version 2. (0x02)
	unsigned char version;
	
	uint32_t artist_start;	// File Offset to the artist table(starting from 0)
	uint32_t album_start;	// File Offset to the album table(starting from 0)
	uint32_t song_start;		// File Offset of the song table(starting from 0)
	uint32_t file_start;		// File Offset to the filename table(starting from 0)

	uint32_t artist_count;	// Number of artists
	uint32_t album_count;	// Number of albums
	uint32_t song_count;		// Number of songs
	uint32_t file_count;		// Number of File Entries, this is needed for the binary search.

	uint32_t artist_len;		// Max Length of the artist name field
	uint32_t album_len;		// Max Length of the album name field
	uint32_t song_len;		// Max Length of the song name field
	uint32_t genre_len;		// Max Length of the genre field
	uint32_t file_len;		// Max Length of the filename field.

	uint32_t song_array_count;	// Number of entries in songs-per-album array
	uint32_t album_array_count;	// Number of entries in albums-per-artist array

	struct {
		unsigned reserved : 31;		// must be 0
		unsigned rundb_dirty : 1;	// if the TagDatabase in unsynchronized with the RuntimeDatabase, 0 if synchronized.
	} flags;
};

int header_write(FILE *fd, const struct header *header);

#endif

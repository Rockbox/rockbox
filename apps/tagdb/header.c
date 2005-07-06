
#include <stdio.h>

#include "header.h"

int header_write(FILE *fd, const struct header *h) {
// Write the header to file
	uint32_t be;
	
	if( fwrite(h->magic, 3, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write magic[3]\n");
		return ERR_FILE;
	}
	if( fwrite(&h->version, 1, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write version\n");
		return ERR_FILE;
	}
	
	be = BE32(h->artist_start);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write artist_start\n");
		return ERR_FILE;
	}
	
	be = BE32(h->album_start);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write album_start\n");
		return ERR_FILE;
	}
	
	be = BE32(h->song_start);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write song_start\n");
		return ERR_FILE;
	}
	
	be = BE32(h->file_start);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write file_start\n");
		return ERR_FILE;
	}
	
	
	be = BE32(h->artist_count);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write artist_count\n");
		return ERR_FILE;
	}
	
	be = BE32(h->album_count);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write album_count\n");
		return ERR_FILE;
	}
	
	be = BE32(h->song_count);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write song_count\n");
		return ERR_FILE;
	}
	
	be = BE32(h->file_count);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write file_count\n");
		return ERR_FILE;
	}
	
	
	be = BE32(h->artist_len);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write artist_len\n");
		return ERR_FILE;
	}
	
	be = BE32(h->album_len);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write album_len\n");
		return ERR_FILE;
	}
	
	be = BE32(h->song_len);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write song_len\n");
		return ERR_FILE;
	}
	
	be = BE32(h->genre_len);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write genre_len\n");
		return ERR_FILE;
	}
	
	be = BE32(h->file_len);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write file_len\n");
		return ERR_FILE;
	}

		
	be = BE32(h->song_array_count);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write song_array_count\n");
		return ERR_FILE;
	}
	
	be = BE32(h->album_array_count);
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write album_array_count\n");
		return ERR_FILE;
	}
	
	
	be = BE32( (h->flags.reserved << 1) | (h->flags.rundb_dirty) );
	if( fwrite(&be, 4, 1, fd) != 1 ) {
		DEBUGF("header_write: failed to write flags\n");
		return ERR_FILE;
	}
	
	
	return ERR_NONE;
}

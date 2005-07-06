#include "malloc.h" // realloc() and free()
#include <string.h> // strncasecmp()

#include "song.h"

// how is our flag organized?
#define FLAG ( 0xCF )
#define FLAG_VALID(flag) (flag == 0xCF)

static int do_resize(struct song_entry *e, const uint32_t name_len, const uint32_t genre_len, const int zero_fill);

struct song_entry* new_song_entry(const uint32_t name_len, const uint32_t genre_len) {
        // Start my allocating memory
	struct song_entry *e = (struct song_entry*)malloc(sizeof(struct song_entry));
	if( e == NULL ) {
		DEBUGF("new_song_entry: could not allocate memory\n");
		return NULL;
	}
	
	// We begin empty
	e->name = NULL;
	e->size.name_len = 0;
	
	e->artist = 0;
	e->album = 0;
	e->file = 0;
	
	e->genre = NULL;
	e->size.genre_len = 0;

	e->bitrate = 0;
	e->year = 0;
	e->playtime = 0;
	e->track = 0;
	e->samplerate = 0;

	e->flag = FLAG;

	// and resize to the requested size
	if( do_resize(e, name_len, genre_len, 1) ) {
		free(e);
		return NULL;
	}
	return e;
}

int song_entry_destruct(struct song_entry *e) {
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));

	free(e->name);
	free(e->genre);

	free(e);
	
	return ERR_NONE;
}

static int do_resize(struct song_entry *e, const uint32_t name_len, const uint32_t genre_len, const int zero_fill) {
	void* temp;

	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	// begin with name
	if( name_len != e->size.name_len ) {
		temp = realloc(e->name, name_len);
		if(temp == NULL && name_len > 0) {       // if realloc(,0) don't complain about NULL-pointer
			DEBUGF("song_entry_resize: out of memory to resize name\n");
			return ERR_MALLOC;
		}
		e->name = (char*)temp;
		
		// if asked, fill it with zero's
		if( zero_fill ) {
			uint32_t i;
			for(i=e->size.name_len; i<name_len; i++)
				e->name[i] = (char)0x00;
		}
		
		e->size.name_len = name_len;
	}
	
	// now the genre
	if( genre_len != e->size.genre_len ) {
		temp = realloc(e->genre, genre_len);
		if(temp == NULL && genre_len > 0) {      // if realloc(,0) don't complain about NULL-pointer
			DEBUGF("song_entry_resize: out of memory to resize genre\n");
			return ERR_MALLOC;
		}
		e->genre = (char*)temp;
		 
		// if asked, fill it with zero's
		if( zero_fill ) {
			uint32_t i;
			for(i=e->size.genre_len; i<genre_len; i++)
				e->genre[i] = (char)0x00;
		}
		
		e->size.genre_len = genre_len;
	}

	return ERR_NONE;
}

inline int song_entry_resize(struct song_entry *e, const uint32_t name_len, const uint32_t genre_len) {	
	return do_resize(e, name_len, genre_len, 1);
}

int song_entry_serialize(FILE *fd, const struct song_entry *e) {
	assert(fd != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));

	// First byte we write is a flag-byte to indicate this is a valid record
	if( fwrite(&e->flag, 1, 1, fd) != 1 ) {
		DEBUGF("song_entry_serialize: failed to write flag-byte\n");
		return ERR_FILE;
	}
	
	// Write the length of the name field
	if( fwrite(&e->size.name_len, sizeof(e->size.name_len), 1, fd) != 1 ) {
		DEBUGF("song_entry_serialize: failed to write name_len\n");
		return ERR_FILE;
	}

	// now the name field itself
	if( fwrite(e->name, 1, e->size.name_len, fd) != e->size.name_len ) {
		DEBUGF("song_entry_serialize: failed to write name\n");
		return ERR_FILE;
	}

	// Artist field
	if( fwrite(&e->artist, sizeof(e->artist), 1, fd) != 1 ) {
		DEBUGF("song_entry_serialize: failed to write artist\n");
		return ERR_FILE;
	}

	// Album field
	if( fwrite(&e->album, sizeof(e->album), 1, fd) != 1 ) {
		DEBUGF("song_entry_serialize: failed to write album\n");
		return ERR_FILE;
	}
	
	// File field
	if( fwrite(&e->file, sizeof(e->file), 1, fd) != 1 ) {
		DEBUGF("song_entry_serialize: failed to write file\n");
		return ERR_FILE;
	}

	// length of genre field
	if( fwrite(&e->size.genre_len, sizeof(e->size.genre_len), 1, fd) != 1 ) {
		DEBUGF("song_entry_serialize: failed to write genre_len\n");
		return ERR_FILE;
	}

	// genre itself
	if( fwrite(e->genre, 1, e->size.genre_len, fd) != e->size.genre_len ) {
		DEBUGF("song_entry_serialize: failed to write genre\n");
		return ERR_FILE;
	}

	// Bitrate field
	if( fwrite(&e->bitrate, sizeof(e->bitrate), 1, fd) != 1 ) {
		DEBUGF("song_entry_serialize: failed to write bitrate\n");
		return ERR_FILE;
	}

	// Year field
	if( fwrite(&e->year, sizeof(e->year), 1, fd) != 1 ) {
		DEBUGF("song_entry_serialize: failed to write year\n");
		return ERR_FILE;
	}

	// Playtime field
	if( fwrite(&e->playtime, sizeof(e->playtime), 1, fd) != 1 ) {
		DEBUGF("song_entry_serialize: failed to write playtime\n");
		return ERR_FILE;
	}

	// Track field
	if( fwrite(&e->track, sizeof(e->track), 1, fd) != 1 ) {
		DEBUGF("song_entry_serialize: failed to write track\n");
		return ERR_FILE;
	}

	// Samplerate field
	if( fwrite(&e->samplerate, sizeof(e->samplerate), 1, fd) != 1 ) {
		DEBUGF("song_entry_serialize: failed to write samplerate\n");
		return ERR_FILE;
	}

	return ERR_NONE;
}

int song_entry_unserialize(struct song_entry **dest, FILE *fd) {
	uint32_t length;
	struct song_entry* e;

	assert(dest != NULL);
	assert(fd != NULL);

	// Allocate memory
	e = new_song_entry(0, 0);
	if( e == NULL ) {
		DEBUGF("song_entry_unserialize: could not create new song_entry\n");
		return ERR_MALLOC;
	}
	
	// First we read the length of the name field
	if( fread(&length, sizeof(length), 1, fd) != 1 ) {
		DEBUGF("song_entry_unserialize: failed to read name_len\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}

	// allocate memory for the upcomming name-field
	if( do_resize(e, length, 0, 0) ) {
		DEBUGF("song_entry_unserialize: failed to allocate memory for name\n");
		song_entry_destruct(e);
		return ERR_MALLOC;
	}

	// read it in
	if( fread(e->name, 1, e->size.name_len, fd) != e->size.name_len ) {
		DEBUGF("song_entry_unserialize: failed to read name\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}
	
	// Artist field
	if( fread(&e->artist, sizeof(e->artist), 1, fd) != 1 ) {
		DEBUGF("song_entry_unserialize: failed to read artist\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}

	// Album field
	if( fread(&e->album, sizeof(e->album), 1, fd) != 1 ) {
		DEBUGF("song_entry_unserialize: failed to read album\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}

	// File field
	if( fread(&e->file, sizeof(e->file), 1, fd) != 1 ) {
		DEBUGF("song_entry_unserialize: failed to read file\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}

	// Next the length of genre
	if( fread(&length, sizeof(length), 1, fd) != 1 ) {
		DEBUGF("song_entry_unserialize: failed to read genre_len\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}

	// allocate memory for the upcomming name-field
	if( do_resize(e, e->size.name_len, length, 0) ) {
		DEBUGF("song_entry_unserialize: failed to allocate memory for song\n");
		song_entry_destruct(e);
		return ERR_MALLOC;
	}

	// read it in
	if( fread(e->genre, 1, e->size.genre_len, fd) != e->size.genre_len ) {
		DEBUGF("song_entry_unserialize: failed to read genre\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}

	// Bitrate field
	if( fread(&e->bitrate, sizeof(e->bitrate), 1, fd) != 1 ) {
		DEBUGF("song_entry_unserialize: failed to read bitrate\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}

	// Year field
	if( fread(&e->year, sizeof(e->year), 1, fd) != 1 ) {
		DEBUGF("song_entry_unserialize: failed to read year\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}

	// Playtime field
	if( fread(&e->playtime, sizeof(e->playtime), 1, fd) != 1 ) {
		DEBUGF("song_entry_unserialize: failed to read playtime\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}

	// Track field
	if( fread(&e->track, sizeof(e->track), 1, fd) != 1 ) {
		DEBUGF("song_entry_unserialize: failed to read track\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}

	// Samplerate field
	if( fread(&e->samplerate, sizeof(e->samplerate), 1, fd) != 1 ) {
		DEBUGF("song_entry_unserialize: failed to read samplerate\n");
		song_entry_destruct(e);
		return ERR_FILE;
	}

	*dest = e;
	return ERR_NONE;
}

int song_entry_write(FILE *fd, struct song_entry *e, struct song_size *s) {
	uint32_t be32;
	uint16_t be16;
	char pad = 0x00;
	
	assert(fd != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));

	// song name
	if( fwrite(e->name, 1, e->size.name_len, fd) != e->size.name_len ) {
		DEBUGF("song_entry_write: failed to write name\n");
		return ERR_FILE;
	}
	// pad the rest (abuse be32 for counter)
	be32 = e->size.name_len;
	while( s != NULL && s->name_len > be32) {
		if( fwrite(&pad, 1, 1, fd) == 1 ) {
			be32++;
		} else {
			DEBUGF("genre_entry_write: failed to pad name\n");
			return ERR_FILE;
		}
	}

	// artist
	be32 = BE32(e->artist);
	if( fwrite(&be32, sizeof(be32), 1, fd) != 1 ) {
		DEBUGF("song_entry_write: failed to write artist\n");
		return ERR_FILE;
	}

	// album
	be32 = BE32(e->album);
	if( fwrite(&be32, sizeof(be32), 1, fd) != 1 ) {
		DEBUGF("song_entry_write: failed to write album\n");
		return ERR_FILE;
	}

	// file
	be32 = BE32(e->file);
	if( fwrite(&be32, sizeof(be32), 1, fd) != 1 ) {
		DEBUGF("song_entry_write: failed to write file\n");
		return ERR_FILE;
	}

	// genre
	if( fwrite(e->genre, 1, e->size.genre_len, fd) != e->size.genre_len ) {
		DEBUGF("song_entry_write: failed to write genre\n");
		return ERR_FILE;
	}
	// pad the rest (abuse be32 for counter)
	be32 = e->size.genre_len;
	while( s != NULL && s->genre_len > be32) {
		if( fwrite(&pad, 1, 1, fd) == 1 ) {
			be32++;
		} else {
			DEBUGF("genre_entry_write: failed to pad genre\n");
			return ERR_FILE;
		}
	}

	// bitrate
	be16 = BE16(e->bitrate);
	if( fwrite(&be16, sizeof(be16), 1, fd) != 1 ) {
		DEBUGF("song_entry_write: failed to write bitrate\n");
		return ERR_FILE;
	}

	// year
	be16 = BE16(e->year);
	if( fwrite(&be16, sizeof(be16), 1, fd) != 1 ) {
		DEBUGF("song_entry_write: failed to write year\n");
		return ERR_FILE;
	}

	// playtime
	be32 = BE32(e->playtime);
	if( fwrite(&be32, sizeof(be32), 1, fd) != 1 ) {
		DEBUGF("song_entry_write: failed to write playtime\n");
		return ERR_FILE;
	}

	// track
	be16 = BE16(e->track);
	if( fwrite(&be16, sizeof(be16), 1, fd) != 1 ) {
		DEBUGF("song_entry_write: failed to write track\n");
		return ERR_FILE;
	}

	// samplerate
	be16 = BE16(e->samplerate);
	if( fwrite(&be16, sizeof(be16), 1, fd) != 1 ) {
		DEBUGF("song_entry_write: failed to write samplerate\n");
		return ERR_FILE;
	}

	return ERR_NONE;
}

inline int song_entry_compare(const struct song_entry *a, const struct song_entry *b) {
	assert(a != NULL);
	assert(b != NULL);
	return strncasecmp(a->name, b->name, (a->size.name_len <= b->size.name_len ? a->size.name_len : b->size.name_len) );
}
    
struct song_size* new_song_size() {
	struct song_size *s;
	s = (struct song_size*)malloc(sizeof(struct song_size));
	if( s == NULL ) {
		DEBUGF("new_song_size: failed to allocate memory\n");
		return NULL;
	}
	s->name_len = 0;
	s->genre_len = 0;
	
	return s;
}

inline uint32_t song_size_get_length(const struct song_size *size) {
	assert(size != NULL);
	return size->name_len + size->genre_len + 6*4;
}

inline int song_size_max(struct song_size *s, const struct song_entry *e) {
	assert(s != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	s->name_len  = ( s->name_len  >= e->size.name_len  ? s->name_len  : e->size.name_len );
	s->genre_len = ( s->genre_len >= e->size.genre_len ? s->genre_len : e->size.genre_len );
	return ERR_NONE;
}

int song_size_destruct(struct song_size *s) {
	assert(s != NULL);
	// nothing to do...
	free(s);
	return ERR_NONE;
}

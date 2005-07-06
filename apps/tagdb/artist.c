#include "malloc.h" // realloc() and free()
#include <string.h> // strncasecmp()

#include "artist.h"

// how is our flag organized?
#define FLAG(deleted, spare) ( 0xC0 | (deleted?0x10:0x00) | (spare & 0x0F) )
#define FLAG_VALID(flag) ((flag & 0xE0) == 0xC0)
#define FLAG_DELETED(flag) (flag & 0x10)
#define FLAG_SPARE(flag) (flag & 0x0F)

static int do_resize(struct artist_entry *e, const uint32_t name_len, const uint32_t album_count, const int zero_fill);

struct artist_entry* new_artist_entry(const uint32_t name_len, const uint32_t album_count) {
	// start by allocating memory
	struct artist_entry *e = (struct artist_entry*)malloc(sizeof(struct artist_entry));
	if( e == NULL ) {
		DEBUGF("new_artist_entry: could not allocate memory\n");
		return NULL;
	}
		
	// We begin empty
	e->name = NULL;
	e->size.name_len = 0;
	e->album = NULL;
	e->size.album_count = 0;
	e->flag = FLAG(0, 0);

	// and resize to the requested size
	if( do_resize(e, name_len, album_count, 1) ) {
		free(e);
		return NULL;
	}
	return e;                                                         
}

int artist_entry_destruct(struct artist_entry *e) {
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	free(e->name);
	free(e->album);
	
	free(e);
	
	return ERR_NONE;
}

static int do_resize(struct artist_entry *e, const uint32_t name_len, const uint32_t album_count, const int zero_fill) {
	void* temp;
	
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	// begin with name
	if( name_len != e->size.name_len ) {
		temp = realloc(e->name, name_len);
		if(temp == NULL && name_len > 0) {       // if realloc(,0) don't complain about NULL-pointer
			DEBUGF("artist_entry_resize: out of memory to resize name\n");
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
	
	// now the album
	if( album_count != e->size.album_count ) {
		temp = realloc(e->album, album_count * sizeof(*e->album));
		if(temp == NULL && album_count > 0) {      // if realloc(,0) don't complain about NULL-pointer
			DEBUGF("artist_entry_resize: out of memory to resize album[]\n");
			return ERR_MALLOC;
		}
		e->album = (uint32_t*)temp;
		 
		// if asked, fill it with zero's
		if( zero_fill ) {
			uint32_t i;
			for(i=e->size.album_count; i<album_count; i++)
				e->album[i] = (uint32_t)0x00000000;
		}
		
		e->size.album_count = album_count;
	}

	return ERR_NONE;
}

int artist_entry_resize(struct artist_entry *e, const uint32_t name_len, const uint32_t album_count) {
	return do_resize(e, name_len, album_count, 1);
}

int artist_entry_serialize(FILE *fd, const struct artist_entry *e) {
	assert(fd != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	if( FLAG_DELETED(e->flag) ) { // we are deleted, do nothing
		return ERR_NONE;
	}
	
	// First byte we write is a flag-byte to indicate this is a valid record
	if( fwrite(&e->flag, 1, 1, fd) != 1 ) {
		DEBUGF("artist_entry_serialize: failed to write flag-byte\n");
		return ERR_FILE;
	}
	
	// First we write the length of the name field
	if( fwrite(&e->size.name_len, sizeof(e->size.name_len), 1, fd) != 1 ) {
		DEBUGF("artist_entry_serialize: failed to write name_len\n");
		return ERR_FILE;
	}

	// now the name field itself
	if( fwrite(e->name, 1, e->size.name_len, fd) != e->size.name_len ) {
		DEBUGF("artist_entry_serialize: failed to write name\n");
		return ERR_FILE;
	}

	// count of albums
	if( fwrite(&e->size.album_count, sizeof(e->size.album_count), 1, fd) != 1 ) {
		DEBUGF("artist_entry_serialize: failed to write album_count\n");
		return ERR_FILE;
	}

	// album[] itself
	if( fwrite(e->album, sizeof(*e->album), e->size.album_count, fd) != e->size.album_count ) {
		DEBUGF("artist_entry_serialize: failed to write albums\n");
		return ERR_FILE;
	}

	return ERR_NONE;
}

int artist_entry_unserialize(struct artist_entry **e, FILE *fd) {
	uint32_t length;
	unsigned char flag;

	assert(e != NULL);
	assert(fd != NULL);

	// First byte we read is flag-byte
	if( fread(&flag, 1, 1, fd) != 1 ) {
		DEBUGF("artist_entry_unserialize: failed to read flag-byte\n");
		return ERR_FILE;
	}

	// See what we have:
	if( ! FLAG_VALID(flag) ) {
		DEBUGF("artist_entry_unserialize: flag-byte not found\n");
		return ERR_INVALID;
	}
	
	// Allocate memory
	*e = new_artist_entry(0, 0);
	if( *e == NULL ) {
		DEBUGF("artist_entry_unserialize: could not create new artist_entry\n");
		return ERR_MALLOC;
	}
	
	(*e)->flag = flag;

	// First we read the length of the name field
	if( fread(&length, sizeof(length), 1, fd) != 1 ) {
		DEBUGF("artist_entry_unserialize: failed to read name_len\n");
		artist_entry_destruct(*e);
		return ERR_FILE;
	}

	// allocate memory for the upcomming name-field
	if( do_resize((*e), length, 0, 0) ) {
		DEBUGF("artist_entry_unserialize: failed to allocate memory for name\n");
		artist_entry_destruct(*e);
		return ERR_MALLOC;
	}

	// read it in
	if( fread((*e)->name, 1, (*e)->size.name_len, fd) != (*e)->size.name_len ) {
		DEBUGF("artist_entry_unserialize: failed to read name\n");
		artist_entry_destruct(*e);
		return ERR_FILE;
	}

	if( FLAG_DELETED(flag) ) {
		// all there is... free some memory
		if( do_resize(*e, 0, 0, 0) ) {
			DEBUGF("artist_entry_unserialize: couldn't free() name\n");
			return ERR_MALLOC;
		}
		return ERR_NONE;
	}
	
	// Next the count of albums
	if( fread(&length, sizeof(length), 1, fd) != 1 ) {
		DEBUGF("artist_entry_unserialize: failed to read album_count\n");
		artist_entry_destruct(*e);
		return ERR_FILE;
	}

	// allocate memory for the upcomming name-field
	if( do_resize(*e, (*e)->size.name_len, length, 0) ) {
		DEBUGF("artist_entry_unserialize: failed to allocate memory for album[]\n");
		artist_entry_destruct(*e);
		return ERR_MALLOC;
	}

	// read it in
	if( fread((*e)->album, sizeof(*(*e)->album), (*e)->size.album_count, fd) != (*e)->size.album_count ) {
		DEBUGF("artist_entry_unserialize: failed to read albums\n");
		artist_entry_destruct(*e);
		return ERR_FILE;
	}

	return ERR_NONE;
}

int artist_entry_write(FILE *fd, const struct artist_entry *e, const struct artist_size *s) {
	uint32_t i, be;
	char pad = 0x00;

	assert(fd != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));

	if( FLAG_DELETED(e->flag) ) { // we are deleted, do nothing
		return ERR_NONE;
	}
	
	// artist name
	if( fwrite(e->name, 1, e->size.name_len, fd) != e->size.name_len ) {
		DEBUGF("artist_entry_write: failed to write name\n");
		return ERR_FILE;
	}
	// padd the rest
	i = e->size.name_len;
	while( s != NULL && s->name_len > i) {
		if( fwrite(&pad, 1, 1, fd) == 1 ) {
			i++;
			continue;
		} else {
			DEBUGF("artist_entry_write: failed to padd name\n");
			return ERR_FILE;
		}
	}

	// album offsets, but in BIG ENDIAN!
	// so we need to iterate over each item to convert it
	for(i=0; i<e->size.album_count; i++) {
		be = BE32(e->album[i]);
		if( fwrite(&be, sizeof(be), 1, fd) != 1 ) {
			DEBUGF("artist_entry_write: failed to write album[%d]\n", i);
			return ERR_FILE;
		}
	}
	// padd the rest
	be = BE32(0x00000000);
	for(; s != NULL && i<s->album_count; i++) {
		if( fwrite(&be, sizeof(be), 1, fd) != 1 ) {
			DEBUGF("artist_entry_write: failed to padd album[]\n");
			return ERR_FILE;
		}
	}

	return ERR_NONE;
}

inline int artist_entry_compare(const struct artist_entry *a, const struct artist_entry *b) {
	assert(a != NULL);
	assert(b != NULL);
	if( a->name == NULL || b->name == NULL )
		return 1; // never match on no-names
	return strcasecmp(a->name, b->name);
}

struct artist_size* new_artist_size() {
	struct artist_size *s;
	s = (struct artist_size*)malloc(sizeof(struct artist_size));
	if( s == NULL ) {
		DEBUGF("new_artist_size: failed to allocate memory\n");
		return NULL;
	}
	s->name_len = 0;
	s->album_count = 0;
	
	return s;
}

inline uint32_t artist_size_get_length(const struct artist_size *size) {
	assert(size != NULL);
	return size->name_len + 4*size->album_count;
}

inline int artist_size_max(struct artist_size *s, const struct artist_entry *e) {
	s->name_len    = ( s->name_len    >= e->size.name_len    ? s->name_len    : e->size.name_len );
	s->album_count = ( s->album_count >= e->size.album_count ? s->album_count : e->size.album_count );
	return ERR_NONE;	
}

int artist_size_destruct(struct artist_size *s) {
	assert(s != NULL);
	// nothing to do...
	free(s);
	return ERR_NONE;
}

int artist_entry_add_album_mem(struct artist_entry *e, struct artist_size *s, const uint32_t album) {
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));

	if( do_resize(e, e->size.name_len, e->size.album_count+1, 0) ) {
		DEBUGF("artist_entry_add_song_mem: failed to resize album[]\n");
		return ERR_MALLOC;
	}

	e->album[e->size.album_count-1] = album;

	if(s != NULL) artist_size_max(s, e); // can't fail

	return ERR_NONE;
}

static int delete_serialized(FILE *fd, struct artist_entry *e) {
// the entry should be both, in memory and in file at the current location
// this function will mark the file-entry as deleted
	uint32_t size;
	unsigned char flag;
	// overwrite the beginning of the serialized data:
	assert(fd != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	flag = FLAG(1, 0); // mark as deleted

	// First byte we write is the flag-byte to indicate this is a deleted
	if( fwrite(&flag, 1, 1, fd) != 1 ) {
		DEBUGF("artist_entry_delete_serialized: failed to write flag-byte\n");
		return ERR_FILE;
	}
	
	// Then we write the length of the COMPLETE entry
	size = artist_size_get_length(&e->size) + 4; // 4 = overhead for the album[]
	if( fwrite(&size, sizeof(size), 1, fd) != 1 ) {
		DEBUGF("artist_entry_delete_serialized: failed to write len\n");
		return ERR_FILE;
	}

	return ERR_NONE;
}

int artist_entry_add_album_file(FILE *fd, struct artist_entry *e, struct artist_size *s, const uint32_t album) {
	assert(fd != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));

	DEBUGF("artist_entry_add_song_file() called\n");

	if( delete_serialized(fd, e) ) {
		DEBUGF("artist_entry_add_album_file: could not mark as deleted\n");
		return ERR_FILE;
	}
	
	return ERR_NO_INPLACE_UPDATE;
}

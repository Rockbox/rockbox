#include "malloc.h" // realloc() and free()
#include <strings.h> // strncasecmp()
#include <string.h> // strlen()

#include "album.h"

// how is our flag organized?
#define FLAG(deleted, spare) ( 0xE0 | (deleted?0x10:0x00) | (spare & 0x0F) )
#define FLAG_VALID(flag) ((flag & 0xE0) == 0xE0)
#define FLAG_DELETED(flag) (flag & 0x10)
#define FLAG_SPARE(flag) (flag & 0x0F)

static int do_resize(struct album_entry *e, const uint32_t name_len, const uint32_t song_count, const int zero_fill);

struct album_entry* new_album_entry(const uint32_t name_len, const uint32_t song_count) {
	// Start my allocating memory
	struct album_entry *e = (struct album_entry*)malloc(sizeof(struct album_entry));
	if( e == NULL ) {
		DEBUGF("new_album_entry: could not allocate memory\n");
		return NULL;
	}
	
	// We begin empty
	e->name = NULL;
	e->size.name_len = 0;
	e->key = NULL;
	e->artist = 0;
	e->song = NULL;
	e->size.song_count = 0;

	e->flag = FLAG(0, 0);

	// and resize to the requested size
	if( do_resize(e, name_len, song_count, 1) ) {
		free(e);
		return NULL;
	}
	return e;
}

int album_entry_destruct(struct album_entry *e) {
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	free(e->name);
	free(e->key);
	free(e->song);

	free(e);
	
	return ERR_NONE;
}

static int do_resize(struct album_entry *e, const uint32_t name_len, const uint32_t song_count, const int zero_fill) {
	void* temp;
	
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	// begin with name
	if( name_len != e->size.name_len ) {
		temp = realloc(e->name, name_len);
		if(temp == NULL && name_len > 0) {	// if realloc(,0) don't complain about NULL-pointer
			DEBUGF("do_resize: out of memory to resize name\n");
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
	
	// now the song[]
	if( song_count != e->size.song_count ) {
		temp = realloc(e->song, song_count * sizeof(*e->song));
		if(temp == NULL && song_count > 0) {	// if realloc(,0) don't complain about NULL-pointer
			DEBUGF("album_entry_resize: out of memory to resize song[]\n");
			return ERR_MALLOC;
		}
		e->song = (uint32_t*)temp;
		 
		// if asked, fill it with zero's
		if( zero_fill ) {
			uint32_t i;
			for(i=e->size.song_count; i<song_count; i++)
				e->song[i] = (uint32_t)0x00000000;
		}
		
		e->size.song_count = song_count;
	}

	return ERR_NONE;
}

inline int album_entry_resize(struct album_entry *e, const uint32_t name_len, const uint32_t song_count) {
	return do_resize(e, name_len, song_count, 1);
}

int album_entry_serialize(FILE *fd, const struct album_entry *e) {
	uint32_t length;
	
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	assert(fd != NULL);
	
	if( FLAG_DELETED(e->flag) ) { // we are deleted, do nothing
		return ERR_NONE;
	}

	// First byte we write is a flag-byte
	if( fwrite(&e->flag, 1, 1, fd) != 1 ) {
		DEBUGF("album_entry_serialize: failed to write flag-byte\n");
		return ERR_FILE;
	}
	
	// First we write the length of the name field
	if( fwrite(&e->size.name_len, sizeof(e->size.name_len), 1, fd) != 1 ) {
		DEBUGF("album_entry_serialize: failed to write name_len\n");
		return ERR_FILE;
	}

	// now the name field itself
	if( fwrite(e->name, 1, e->size.name_len, fd) != e->size.name_len ) {
		DEBUGF("album_entry_serialize: failed to write name\n");
		return ERR_FILE;
	}

	// the key-field (if present)
	if( e->key != NULL ) {
		length = strlen(e->key);
	} else {
		length = 0;
	}
	// length (always, 0 if not present)
	if( fwrite(&length, sizeof(length), 1, fd) != 1 ) {
		DEBUGF("album_entry_serialize: failed to write length of key\n");
		return ERR_FILE;
	}
	if( e->key != NULL ) {
		// key itself
		if( fwrite(e->key, 1, length, fd) != length ) {
			DEBUGF("album_entry_serialize: failed to write key\n");
			return ERR_FILE;
		}
	}

	// Artist field
	if( fwrite(&e->artist, sizeof(e->artist), 1, fd) != 1 ) {
		DEBUGF("album_entry_serialize: failed to write artist\n");
		return ERR_FILE;
	}

	// count of songs
	if( fwrite(&e->size.song_count, sizeof(e->size.song_count), 1, fd) != 1 ) {
		DEBUGF("album_entry_serialize: failed to write song_count\n");
		return ERR_FILE;
	}

	// song[] itself
	if( fwrite(e->song, sizeof(*e->song), e->size.song_count, fd) != e->size.song_count ) {
		DEBUGF("album_entry_serialize: failed to write songs\n");
		return ERR_FILE;
	}

	return ERR_NONE;
}

int album_entry_unserialize(struct album_entry **e, FILE *fd) {
	uint32_t length;
	unsigned char flag;
	
	assert(e != NULL);
	assert(fd != NULL);
	
	// First byte we read are the flags
	if( fread(&flag, 1, 1, fd) != 1 ) {
		DEBUGF("album_entry_unserialize: failed to read flag-byte\n");
		return ERR_FILE;
	}

	// See what we have:
	if( ! FLAG_VALID(flag) ) {
		DEBUGF("album_entry_unserialize: flag-byte is invalid\n");
		return ERR_INVALID;
	}
	
	// Allocate memory
	*e = new_album_entry(0, 0);
	if( *e == NULL ) {
		DEBUGF("album_entry_unserialize: could not create new album_entry\n");
		return ERR_MALLOC;
	}

	(*e)->flag = flag; // we had a valid entry, copy it over

	// First we read the length of the name field
	if( fread(&length, sizeof(length), 1, fd) != 1 ) {
		DEBUGF("album_entry_unserialize: failed to read name_len\n");
		album_entry_destruct(*e);
		return ERR_FILE;
	}

	// allocate memory for the upcomming name-field
	if( do_resize(*e, length, 0, 0) ) {
		DEBUGF("album_entry_unserialize: failed to allocate memory for name\n");
		album_entry_destruct(*e);
		return ERR_MALLOC;
	}

	// read it in
	if( fread((*e)->name, 1, (*e)->size.name_len, fd) != (*e)->size.name_len ) {
		DEBUGF("album_entry_unserialize: failed to read name\n");
		album_entry_destruct(*e);
		return ERR_FILE;
	}

	if( FLAG_DELETED(flag) ) {
		// all there is... free some memory
		if( do_resize(*e, 0, 0, 0) ) {
			DEBUGF("album_entry_unserialize: couldn't free() name\n");
			return ERR_MALLOC;
		}
		return ERR_NONE;
	}

	// maybe a key-field
	if( fread(&length, sizeof(length), 1, fd) != 1 ) {
		DEBUGF("album_entry_unserialize: failed to read length of key\n");
		album_entry_destruct(*e);
		return ERR_FILE;
	}
	
	if( length > 0 ) {
		// allocate memory
		if( ((*e)->key = malloc(length)) == NULL ) {
			DEBUGF("album_entry_unserialize: failed to allocate memory for key\n");
			album_entry_destruct(*e);
			return ERR_MALLOC;
		}
	
		// read it
		if( fread((*e)->key, 1, length, fd) != length ) {
			DEBUGF("album_entry_unserialize: failed to read key\n");
			album_entry_destruct(*e);
			return ERR_FILE;
		}
	}

	// next the artist field
	if( fread(&(*e)->artist, sizeof((*e)->artist), 1, fd) != 1 ) {
		DEBUGF("album_entry_unserialize: failed to read artist\n");
		album_entry_destruct(*e);
		return ERR_FILE;
	}

	// Next the count of songs
	if( fread(&length, sizeof(length), 1, fd) != 1 ) {
		DEBUGF("album_entry_unserialize: failed to read song_count\n");
		album_entry_destruct(*e);
		return ERR_FILE;
	}

	// allocate memory for the upcomming name-field
	if( do_resize(*e, (*e)->size.name_len, length, 0) ) {
		DEBUGF("album_entry_unserialize: failed to allocate memory for song[]\n");
		album_entry_destruct(*e);
		return ERR_MALLOC;
	}

	// read it in
	if( fread((*e)->song, sizeof(*(*e)->song), (*e)->size.song_count, fd) != (*e)->size.song_count ) {
		DEBUGF("album_entry_unserialize: failed to read songs\n");
		album_entry_destruct(*e);
		return ERR_FILE;
	}

	return ERR_NONE;
}

int album_entry_write(FILE *fd, struct album_entry *e, struct album_size *s) {
	uint32_t i, be;
	char pad = 0x00;

	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	assert(fd != NULL);
	
	if( FLAG_DELETED(e->flag) ) { // we are deleted, do nothing
		return ERR_NONE;
	}
	
	// resize-write to size *s
	// First check if we are not reducing the size...
	if( s != NULL && ( s->name_len < e->size.name_len || s->song_count < e->size.song_count ) ) {
		// just do it in 2 steps
		if( do_resize(e, s->name_len, s->song_count, 0) ) {
			DEBUGF("album_entry_write: failed to reduce size of entry, failing...\n");
			return ERR_MALLOC;
		}
	}
	
	// album name
	if( fwrite(e->name, 1, e->size.name_len, fd) != e->size.name_len ) {
		DEBUGF("album_entry_write: failed to write name\n");
		return ERR_FILE;
	}
	// pad the rest
	i = e->size.name_len;
	while( s != NULL && s->name_len > i) {
		if( fwrite(&pad, 1, 1, fd) == 1 ) {
			i++;
			continue;
		} else {
			DEBUGF("album_entry_write: failed to pad name\n");
			return ERR_FILE;
		}
	}

	// artist
	be = BE32(e->artist);
	if( fwrite(&be, sizeof(be), 1, fd) != 1 ) {
		DEBUGF("album_entry_write: failed to write artist\n");
		return ERR_FILE;
	}

	// song offsets, but in BIG ENDIAN!
	// so we need to iterate over each item to convert it
	for(i=0; i<e->size.song_count; i++) {
		be = BE32(e->song[i]);
		if( fwrite(&be, sizeof(be), 1, fd) != 1 ) {
			DEBUGF("album_entry_write: failed to write song[%d]\n", i);
			return ERR_FILE;
		}
	}
	// pad the rest
	be = BE32(0x00000000);
	for(; s != NULL && i<s->song_count; i++) {
		if( fwrite(&be, sizeof(be), 1, fd) != 1 ) {
			DEBUGF("album_entry_write: failed to pad song[]\n");
			return ERR_FILE;
		}
	}

	return 0;
}

inline int album_entry_compare(const struct album_entry *a, const struct album_entry *b) {
	assert(a != NULL);
	assert(b != NULL);
	assert(a->key != NULL);
	assert(b->key != NULL);
	return strcasecmp(a->key, b->key);
}

struct album_size* new_album_size() {
	struct album_size *s;
	s = (struct album_size*)malloc(sizeof(struct album_size));
	if( s == NULL ) {
		DEBUGF("new_album_size: failed to allocate memory\n");
		return NULL;
	}
	s->name_len = 0;
	s->song_count = 0;

	return s;
}

inline uint32_t album_size_get_length(const struct album_size *size) {
	assert(size != NULL);
	return size->name_len + 4 + 4*size->song_count;
}

inline int album_size_max(struct album_size *s, const struct album_entry *e) {
	assert(s != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	s->name_len   = ( s->name_len   >= e->size.name_len   ? s->name_len   : e->size.name_len );
	s->song_count = ( s->song_count >= e->size.song_count ? s->song_count : e->size.song_count );
	return ERR_NONE;
}

int album_size_destruct(struct album_size *s) {
	assert(s != NULL);
	// nothing to do...
	free(s);
	return ERR_NONE;
}

int album_entry_add_song_mem(struct album_entry *e, struct album_size *s, const uint32_t song) {
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	if( do_resize(e, e->size.name_len, e->size.song_count+1, 0) ) {
		DEBUGF("album_entry_add_song_mem: failed to resize song[]\n");
		return ERR_MALLOC;
	}

	e->song[e->size.song_count-1] = song;

	if( s != NULL) album_size_max(s, e); // can't fail

	return ERR_NONE;
}

static int delete_serialized(FILE *fd, struct album_entry *e) {
// the entry should be both, in memory and in file at the current location
// this function will mark the file-entry as deleted
	uint32_t size;
	unsigned char flag;
	
	assert(fd != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	// overwrite the beginning of the serialized data:
	flag = FLAG(1, 0); // set the delete flag, clear the spare flags
	
	// First byte we write is the flag-byte to indicate this is a deleted
	if( fwrite(&flag, 1, 1, fd) != 1 ) {
		DEBUGF("album_entry_delete_serialized: failed to write flag-byte\n");
		return ERR_FILE;
	}
	
	// Then we write the length of the COMPLETE entry
	size = album_size_get_length(&e->size) + 4; // 4 = overhead for the song[]
	if( fwrite(&size, sizeof(size), 1, fd) != 1 ) {
		DEBUGF("album_entry_delete_serialized: failed to write len\n");
		return ERR_FILE;
	}

	return ERR_NONE;
}

int album_entry_add_song_file(FILE *fd, struct album_entry *e, struct album_size *s, const uint32_t song) {
	assert(fd != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	DEBUGF("album_entry_add_song_file() called\n");

	if( delete_serialized(fd, e) ) {
		DEBUGF("album_entry_add_song_file: could not mark as deleted\n");
		return ERR_FILE;
	}
	
	return ERR_NO_INPLACE_UPDATE;
}

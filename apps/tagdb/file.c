#include "malloc.h" // realloc() and free()
#include <string.h> // strncasecmp()

#include "file.h"

// how is our flag organized?
#define FLAG ( 0xBF )
#define FLAG_VALID(flag) (flag == 0xBF)

static int do_resize(struct file_entry *e, const uint32_t name_len, const int zero_fill);

struct file_entry* new_file_entry(const uint32_t name_len) {
	// Start my allocating memory
	struct file_entry *e = (struct file_entry*)malloc(sizeof(struct file_entry));
	if( e == NULL ) {
		DEBUGF("new_file_entry: could not allocate memory\n");
		return NULL;
	}
	
	// We begin empty
	e->name = NULL;
	e->size.name_len = 0;
	
	e->hash = 0;
	e->song = 0;
	e->rundb = 0;

	e->flag = FLAG;

	// and resize to the requested size
	if( do_resize(e, name_len, 1) ) {
		free(e);
		return NULL;
	}
	
	return e;
}

int file_entry_destruct(struct file_entry *e) {
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	
	free(e->name);
	
	free(e);

	return ERR_NONE;
}

static int do_resize(struct file_entry *e, const uint32_t name_len, const int zero_fill) {
	void* temp;

	assert(e != NULL);
	assert(FLAG_VALID(e->flag));

	if( name_len != e->size.name_len ) {
		temp = realloc(e->name, name_len);
		if(temp == NULL) {
			DEBUGF("file_entry_resize: out of memory to resize name\n");
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
	
	return ERR_NONE;
}

inline int file_entry_resize(struct file_entry *e, const uint32_t name_len) {
	return do_resize(e, name_len, 1);
}

int file_entry_serialize(FILE *fd, const struct file_entry *e) {
	assert(fd != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));

	// First byte we write is a flag-byte to indicate this is a valid record
	if( fwrite(&e->flag, 1, 1, fd) != 1 ) {
		DEBUGF("file_entry_serialize: failed to write flag-byte\n");
		return ERR_FILE;
	}

	// First we write the length of the name field
	if( fwrite(&e->size.name_len, sizeof(e->size.name_len), 1, fd) != 1 ) {
		DEBUGF("file_entry_serialize: failed to write name_len\n");
		return ERR_FILE;
	}
	// now the name field itself
	if( fwrite(e->name, 1, e->size.name_len, fd) != e->size.name_len ) {
		DEBUGF("file_entry_serialize: failed to write name\n");
		return ERR_FILE;
	}

	// hash field
	if( fwrite(&e->hash, sizeof(e->hash), 1, fd) != 1 ) {
		DEBUGF("file_entry_serialize: failed to write hash\n");
		return ERR_FILE;
	}

	// song field
	if( fwrite(&e->song, sizeof(e->song), 1, fd) != 1 ) {
		DEBUGF("file_entry_serialize: failed to write song\n");
		return ERR_FILE;
	}
	
	// rundb field
	if( fwrite(&e->rundb, sizeof(e->rundb), 1, fd) != 1 ) {
		DEBUGF("file_entry_serialize: failed to write rundb\n");
		return ERR_FILE;
	}

	return ERR_NONE;
}

int file_entry_unserialize(struct file_entry **dest, FILE *fd) {
	uint32_t length;
	struct file_entry *e;

	assert(dest != NULL);
	assert(fd != NULL);

	// Allocate memory
	e = new_file_entry(0);
	if( e == NULL ) {
		DEBUGF("file_entry_unserialize: could not create new file_entry\n");
		return ERR_MALLOC;
	}
	
	// First we read the length of the name field
	if( fread(&length, sizeof(length), 1, fd) != 1 ) {
		DEBUGF("file_entry_unserialize: failed to read name_len\n");
		file_entry_destruct(e);
		return ERR_FILE;
	}

	// allocate memory for the upcomming name-field
	if( do_resize(e, length, 0) ) {
		DEBUGF("file_entry_unserialize: failed to allocate memory for name\n");
		file_entry_destruct(e);
		return ERR_MALLOC;
	}

	// read it in
	if( fread(e->name, 1, e->size.name_len, fd) != e->size.name_len ) {
		DEBUGF("file_entry_unserialize: failed to read name\n");
		file_entry_destruct(e);
		return ERR_FILE;
	}
	
	// hash field
	if( fread(&e->hash, sizeof(e->hash), 1, fd) != 1 ) {
		DEBUGF("file_entry_unserialize: failed to read hash\n");
		file_entry_destruct(e);
		return ERR_FILE;
	}

	// song field
	if( fread(&e->song, sizeof(e->song), 1, fd) != 1 ) {
		DEBUGF("file_entry_unserialize: failed to read song\n");
		file_entry_destruct(e);
		return ERR_FILE;
	}

	// rundb field
	if( fread(&e->rundb, sizeof(e->rundb), 1, fd) != 1 ) {
		DEBUGF("file_entry_unserialize: failed to read rundb\n");
		file_entry_destruct(e);
		return ERR_FILE;
	}

	*dest = e;
	return ERR_NONE;
}

int file_entry_write(FILE *fd, struct file_entry *e, struct file_size *s) {
	uint32_t be32;
	char pad = 0x00;

	assert(fd != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));

	// file name
	if( fwrite(e->name, 1, e->size.name_len, fd) != e->size.name_len ) {
		DEBUGF("file_entry_write: failed to write name\n");
		return ERR_FILE;
	}
	// pad the rest
	be32 = e->size.name_len; // abuse be32 as counter
	while( s != NULL && s->name_len > be32) {
		if( fwrite(&pad, 1, 1, fd) == 1 ) {
			be32++;
		} else {
			DEBUGF("file_entry_write: failed to pad name\n");
			return ERR_FILE;
		}
	}

	// hash
	be32 = BE32(e->hash);
	if( fwrite(&be32, sizeof(be32), 1, fd) != 1 ) {
		DEBUGF("file_entry_write: failed to write hash\n");
		return ERR_FILE;
	}

	// song
	be32 = BE32(e->song);
	if( fwrite(&be32, sizeof(be32), 1, fd) != 1 ) {
		DEBUGF("file_entry_write: failed to write song\n");
		return ERR_FILE;
	}

	// rundb
	be32 = BE32(e->rundb);
	if( fwrite(&be32, sizeof(be32), 1, fd) != 1 ) {
		DEBUGF("file_entry_write: failed to write rundb\n");
		return ERR_FILE;
	}

	return ERR_NONE;
}

inline int file_entry_compare(const struct file_entry *a, const struct file_entry *b) {
	assert(a != NULL);
	assert(b != NULL);
	return strncasecmp(a->name, b->name, (a->size.name_len <= b->size.name_len ? a->size.name_len : b->size.name_len) );
}

struct file_size* new_file_size() {
	struct file_size *s;
	s = (struct file_size*)malloc(sizeof(struct file_size));
	if( s == NULL ) {
		DEBUGF("new_file_size: failed to allocate memory\n");
		return NULL;
	}
	s->name_len = 0;
	
	return s;
}

inline uint32_t file_size_get_length(const struct file_size *size) {
	assert(size != NULL);
	return size->name_len + 12;
}

inline int file_size_max(struct file_size *s, const struct file_entry *e) {
	assert(s != NULL);
	assert(e != NULL);
	assert(FLAG_VALID(e->flag));
	s->name_len   = ( s->name_len   >= e->size.name_len   ? s->name_len   : e->size.name_len );
	return ERR_NONE;
}

int file_size_destruct(struct file_size *s) {
	assert(s != NULL);
	// nothing to do...
	free(s);
	return ERR_NONE;
}

#include "malloc.h" // malloc() and free()

#include "array_buffer.h"
#include "unique.h"

static int add_mem(struct array_buffer *b, void *e);
static int add_file(struct array_buffer *b, void *e);

static int update_entry_mem(struct array_buffer *b, const uint32_t index, uint32_t item);
static int update_entry_file(struct array_buffer *b, const uint32_t index, uint32_t item);

static int find_entry_mem(struct array_buffer *b, const void *needle, uint32_t *index);
static int find_entry_file(struct array_buffer *b, const void *needle, uint32_t *index);

static int sort_mem(struct array_buffer *b);
static int sort_mem_merge_blocks(uint32_t *dest, uint32_t *s1, uint32_t s1_l, uint32_t *s2, uint32_t s2_l, struct array_buffer *b);
static int sort_mem_merge(uint32_t *dest, uint32_t *src, struct array_buffer *b, uint32_t blocksize);
static int sort_file(struct array_buffer *b);

struct array_buffer* new_array_buffer( int      (*cmp)(const void *a, const void *b),
				       int      (*serialize)(FILE *fd, const void *e),
				       int      (*unserialize)(void **e, FILE *fd),
				       uint32_t (*get_length)(const void *size),
				       int      (*write)(FILE *fd, void *e, const void *size),
				       int      (*destruct)(void *e),
				       char*    file_name,
				       void*    max_size,
				       int      (*max_size_update)(void *max_size, const void *e),
				       int      (*max_size_destruct)(void *max_size),
				       int      (*add_item_mem)(void *e, void *s, uint32_t item),
				       int      (*add_item_file)(FILE *fd, void *e, void *s, uint32_t item),
				       int      (*pre_write)(void *e, void *s)
				     ) {
	struct array_buffer *b;
	b = (struct array_buffer*)malloc(sizeof(struct array_buffer));
	if( b == NULL ) {
		DEBUGF("new_array_buffer: failed to allocate memory\n");
		return NULL;
	}
	
	b->count = 0;
	b->array = NULL;
	b->sort = NULL;
	
	b->file_name = file_name;
	
	b->fd = NULL;

	b->cmp = cmp;
	b->serialize = serialize;
	b->unserialize = unserialize;
	b->get_length = get_length;
	b->write = write;
	b->destruct = destruct;
	
	b->max_size = max_size;
	b->max_size_update = max_size_update;
	b->max_size_destruct = max_size_destruct;

	b->add_item_mem = add_item_mem;
	b->add_item_file = add_item_file;

	b->pre_write = pre_write;

	return b;
}

int array_buffer_destruct(struct array_buffer *b, const int free_file_name) {
	assert(b != NULL);
	
	if( b->fd == NULL ) {
		if( b->destruct == NULL ) {
			DEBUGF("array_buffer_destruct: no destruct() function registered\n");
			return ERR_MALLOC;
		}
		//we have memory to clean up
		// iterate over all stored objects:
		for(; b->count > 0; b->count--) {
			if( b->destruct(b->array[b->count-1].mem) ) {
				DEBUGF("array_buffer_destruct: failed to destruct item[%u]\n", b->count-1);
				return ERR_MALLOC;
			}
		}
	}
	free(b->array);

	if( b->fd != NULL ) {
		// we have a file to clean up
		if( fclose(b->fd) != 0 ) {
			DEBUGF("array_buffer_destruct: fclose() failed\n");
			return ERR_FILE;
		}
		b->fd = NULL;

		// remove that file
		if( remove(b->file_name) != 0 ) {
			DEBUGF("array_buffer_destruct: remove() failed\n");
			return ERR_FILE;
		}
	}
	if( free_file_name ) {
		free(b->file_name);
		b->file_name = NULL;
	}

	free(b->sort);
	b->sort = NULL;
	
	// free the max_size
	if( b->max_size != NULL ) {
		if( b->max_size_destruct == NULL ) {
			DEBUGF("array_buffer_destruct: no max_size_destruct() function registered\n");
			return 1;
		}
	
		if( b->max_size_destruct(b->max_size) ) {
			DEBUGF("array_buffer_destruct: failed to destruct max_size\n");
			return ERR_MALLOC;
		}
		b->max_size = NULL;
	}

	free(b);

	return ERR_NONE;
}

int array_buffer_switch_to_file(struct array_buffer *b) {
	uint32_t i;
	long offset;

	assert(b != NULL);
	
	if(b->file_name == NULL) {
		DEBUGF("array_buffer_switch_to_file: no file_name, failing...\n");
		return ERR_MALLOC;
	}

	if( b->fd != NULL ) {
		DEBUGF("array_buffer_switch_to_file: already in file, failing...\n");
		return ERR_MALLOC;
	}

	// function calls exist?
	if( b->serialize == NULL || b->unserialize == NULL ) {
		DEBUGF("array_buffer_switch_to_file: serialize() and/or unserialize() function(s) not registered\n");
		return ERR_INVALID;
	}
	 
	// since we got here, we are VERY short on memory
	// We cannot do any memory allocation before free()ing some
	// The filename is already allocated in the constructor
	
	// open the file
	b->fd = fopen(b->file_name, "w+");
	if( b->fd == NULL ) {
		DEBUGF("array_buffer_switch_to_file: failed to fopen() file\n");
		return ERR_FILE;
	}

	for(i=0; i<b->count; i++) {
		offset = ftell(b->fd);
		if( offset == -1 ) {
			DEBUGF("array_buffer_switch_to_file: ftell() failed\n");
			return ERR_FILE;
		}
			
		if( b->serialize(b->fd, b->array[i].mem) ) {
			DEBUGF("array_buffer_switch_to_file: serialize() failed on item[%u], ignoring...\n", i);
		}
		b->destruct(b->array[i].mem);
		
		b->array[i].file_offset = offset;
	}

	return ERR_NONE;
}

static int add_mem(struct array_buffer *b, void *e) {
	assert(b != NULL);
	assert(e != NULL);
	
	// just copy over the pointer
	b->array[b->count].mem = e;

	return ERR_NONE;
}

static int add_file(struct array_buffer *b, void *e) {
	int rc;

	assert(b != NULL);
	assert(e != NULL);
	
	if( fseek(b->fd, 0, SEEK_END) != 0 ) {
		DEBUGF("add_file: could not seek to end of file\n");
		return ERR_FILE;
	}
	if(( b->array[b->count].file_offset = ftell(b->fd) ) == -1) {
		DEBUGF("add_file: ftell() failed to get file_offset\n");
		return ERR_FILE;
	}
	
	if(( rc = b->serialize(b->fd, e) )) {
		DEBUGF("add_file: could not serialize entry\n");
		return rc;
	}
	if( b->destruct(e) ) {
		DEBUGF("add_file: could not destruct entry, ignoring... (memory leak)\n");
	}
	return ERR_NONE;
}

int array_buffer_add(struct array_buffer *b, void *e, uint32_t *index) {
	void* temp;
	int rc;

	assert(b != NULL);
	assert(e != NULL);

	// allow the object to update the max_size
	// Do this first, so if it fails we can just return without cleanup to do
	if( b->max_size_update != NULL ) {
		if(( rc = b->max_size_update(b->max_size, e) )) {
			DEBUGF("array_buffer_add: could not update max_size, failing...\n");
			return rc;
		}
	}
	
	// we need to enlarge the array[]
	temp = (void*)realloc(b->array, sizeof(*b->array)*(b->count+1));
	while( temp == NULL ) {
		DEBUGF("array_buffer_add: failed to enlarge index_map[]. Switching to file\n");
		if(( rc = array_buffer_switch_to_file(b) )) {
			DEBUGF("array_buffer_add: failed to switch to file, failing...\n");
			return rc;
		}
		// now retry
		temp = (void*)realloc(b->array, sizeof(*b->array)*(b->count+1));
	}
	b->array = (union entry*)temp;

	if( b->fd == NULL ) { // we are in memory
		rc = add_mem(b, e);
		if( rc == ERR_MALLOC ) {
			DEBUGF("array_buffer_add: failed to add in memory due to malloc() trouble, switching to file\n");
			if(( rc = array_buffer_switch_to_file(b) )) {
				DEBUGF("array_buffer_add: failed to switch to file, failing...\n");
				return rc;
			}
			// fall out and catch next if
		}
	} // NOT else, so we can catch the fall-through
	if( b->fd != NULL) {
		if(( rc = add_file(b, e) )) {
			DEBUGF("array_buffer_add: failed to add in file, failing...\n");
			return rc;
		}
	}

	// count and index-stuff
	if(index != NULL) *index = b->count;
	b->count++;

	return ERR_NONE;
}

inline uint32_t array_buffer_get_next_index(struct array_buffer *b) {
	assert( b != NULL );
	return b->count;
}

static int update_entry_mem(struct array_buffer *b, const uint32_t index, const uint32_t item) {
	int rc;

	assert(b != NULL);
	assert(index < b->count);
	
	if( (rc = b->add_item_mem(b->array[index].mem, b->max_size, item)) ) {
		DEBUGF("update_entry_mem: failed to update entry\n");
		return rc;
	}
	
	return ERR_NONE;
}

static int update_entry_file(struct array_buffer *b, const uint32_t index, uint32_t item) {
/*	uint32_t i, index;
	void *e;
	int rc;
	long prev_file_offset;*/

	assert(b != NULL);
	assert(index < b->count);

	printf("TODO: update entry in file\n");

	return 10; // TODO
/*
	rewind(b->fd);
	
	rc = ERR_NOTFOUND;
	for(i=0; i<b->count; i++) {
		prev_file_offset = ftell(b->fd); // keep this file-position
		if( prev_file_offset == -1 ) {
			DEBUGF("file_entry_add_file: ftell() failed\n");
			return ERR_FILE;
		}
		
		if( (rc = b->unserialize(&e, b->fd)) ) {
			DEBUGF("find_entry_add_file: unserialize failed\n");
			return rc;
		}

		if( b->cmp(e, needle) == 0 ) { // found
			if( fseek(b->fd, prev_file_offset, SEEK_SET) ) {
				DEBUGF("file_entry_add_file: fseek() to entry[%u] failed\n", i);
				return ERR_FILE;
			}
			
			rc = b->add_item_file(b->fd, e, b->max_size, item);
			if( !( rc == ERR_NONE || rc == ERR_NO_INPLACE_UPDATE )) {
				DEBUGF("find_entry_add_mem: failed to add item\n");
				return rc;
			}
			
			break; // stop looping
		}
		
		b->destruct(e);
	}
	
	// seek to the end
	if( fseek(b->fd, 0, SEEK_END) != 0) {
		DEBUGF("find_entry_add_file: fseek(SEEK_END) failed\n");
		return ERR_FILE;
	}

	// We either succeded, deleted the entry or didn't find it:
	if( rc == ERR_NOTFOUND ) {
		return rc;	// quit
	} else if( rc == ERR_NONE ) {
		b->destruct(e);	// delete the entry and quit
		return rc;
	}

	// we could not update inplace
	// the entry is deleted, update it and add it again
	if( (rc = b->add_item_mem(e, b->max_size, item)) ) {
		DEBUGF("find_entry_add_file: failed to add item in mem\n");
		return rc;
	}
	
	if( (rc = array_buffer_add(b, e, &index) ) ) {
		DEBUGF("find_entry_add_file: failed to re-add item to array");
		return rc;
	}

	// the entry is now re-added, but with another index number...
	// change the index_map to reflect this:
	b->index_map[i] = index;

	return ERR_NONE;*/
}

int array_buffer_entry_update(struct array_buffer *b, const uint32_t index, uint32_t item) {
	assert(b != NULL);

	if(index >= b->count) {
		DEBUGF("array_buffer_entry_update: index out of bounds\n");
		return ERR_INVALID;
	}
	
	if( b->fd == NULL ) {
		return update_entry_mem(b, index, item);
	} else {
		return update_entry_file(b, index, item);
	}
}

static int find_entry_mem(struct array_buffer *b, const void *needle, uint32_t *index) {
	uint32_t i;

	assert(b != NULL);
	assert(needle != NULL);
	assert(index != NULL);

	for(i=0; i<b->count; i++) {
		if( b->cmp(b->array[i].mem, needle) == 0 ) { // found
			*index = i;
			return ERR_NONE;
		}
	}
	return ERR_NOTFOUND;
}

static int find_entry_file(struct array_buffer *b, const void *needle, uint32_t *index) {
	uint32_t i;
	void *e;
	int rc;
	long prev_file_offset;

	assert(b != NULL);
	assert(needle != NULL);
	assert(index != NULL);
	
	// We do this search in the order of the entries in file.
	// After we found one, we look for the index of that offset
	// (in memory).
	// This will (PROBABELY: TODO) be faster than random-access the file
	rewind(b->fd);
	
	for(i=0; i<b->count; i++) {
		prev_file_offset = ftell(b->fd); // keep this file-position
		if( prev_file_offset == -1 ) {
			DEBUGF("file_entry_add_file: ftell() failed\n");
			return ERR_FILE;
		}
		
		if( (rc = b->unserialize(&e, b->fd)) ) {
			DEBUGF("find_entry_add_file: unserialize failed\n");
			return rc;
		}

		if( b->cmp(e, needle) == 0 ) { // found
			if( fseek(b->fd, prev_file_offset, SEEK_SET) ) {
				DEBUGF("file_entry_add_file: fseek() to entry[%u] failed\n", i);
				return ERR_FILE;
			}
			
			b->destruct(e);
			break; // out of the for() loop
		}
		
		b->destruct(e);
	}

	if( i == b->count ) {
		// we didn't find anything
		return ERR_NOTFOUND;
	}

	// we found an entry, look for the index number of that offset:
	for(i=0; i<b->count; i++) {
		if(prev_file_offset == b->array[i].file_offset) {
			// found
			*index = i;
			return ERR_NONE;
		}
	}

	// we should never get here
	DEBUGF("find_entry_file: found entry in file, but doens't match an index\n");
	return ERR_INVALID;
}

int array_buffer_find_entry(struct array_buffer *b, const void *needle, uint32_t *index) {
	assert(b != NULL);
	assert(needle != NULL);
	assert(index != NULL);	// TODO: if it is null, do the search but trash the index
	
	if( b->fd == NULL ) {
		return find_entry_mem(b, needle, index);
	} else {
		return find_entry_file(b, needle, index);
	}
}

/*
static int sort_mem_merge_blocks(uint32_t *dest, const uint32_t *s1, const uint32_t s1_l, const uint32_t *s2, const uint32_t s2_l, struct array_buffer *b) {
// merges the 2 blocks at s1 (with s1_l items) and s2 (with s2_l items)
// together in dest
	uint32_t *s1_max, s2_max;

#define CMP(a, b) b->cmp( b->entry[a].mem, b->entry[b].mem )
	
	s1_max = s1 + s1_l;
	s2_max = s2 + s2_l;
	while( s1 < s1_max || s2 < s2_max ) {
		while( s1 < s1_max && ( s2 == s2_max || CMP(s1, s2) <= 0 ) ) // s1 is smaller than s2 (or s2 is used up)
			*(dest++) = s1++; // copy and move to next
		while( s2 < s2_max && ( s1 == s1_max || CMP(s1, s2) > 0 ) ) // s2 smaller
			*(dest++) = s2++;
	}

	return ERR_NONE;
}

#define MIN(a, b) ( (a) <= (b) ? (a) : (b) )
static int sort_mem_merge(uint32_t *dest, uint32_t *src, struct array_buffer *b, uint32_t blocksize) {
// does 1 merge from src[] into dest[]
// asumes there are sorted blocks in src[] of size blocksize
	assert( dest != NULL);
	assert( src != NULL );

	assert( b->count > blocksize );
	
	// TODO
}
*/

static int sort_mem(struct array_buffer *b) {
	uint32_t *tmp, blocksize;
	
	assert(b != NULL);

	tmp = (uint32_t*)malloc(sizeof(uint32_t)*b->count);
	if( tmp == NULL ) {
		DEBUGF("sort_mem: could not malloc() for second sort[] array\n");
		return ERR_MALLOC;
	}

	for( blocksize = 1; blocksize < b->count; blocksize++) {
		b->sort[blocksize] = blocksize; // 1-1 map TODO
	}

	free(tmp);

	return ERR_NONE;
}

static int sort_file(struct array_buffer *b) {
	printf("TODO: file-sorting\n"); // TODO
	return ERR_INVALID;
}

int array_buffer_sort(struct array_buffer *b) {
	int rc;
	
	assert(b != NULL);

	b->sort = (uint32_t*)malloc(sizeof(uint32_t)*b->count);
	if( b->sort == NULL ) {
		DEBUGF("array_buffer_sort: could not malloc() sort[] array\n");
		return ERR_MALLOC;
	}

	if( b->fd == NULL ) { // in memory
		rc = sort_mem(b);
		if( rc == ERR_MALLOC ) {
			if(( rc = array_buffer_switch_to_file(b) )) {
				DEBUGF("array_buffer_sort: could not switch to file mode\n");
				return rc;
			}
			return sort_file(b);
		} else if( rc ) {
			DEBUGF("array_buffer_sort: could not sort array\n");
			return rc;
		}
		return ERR_NONE;
	} else {
		return sort_file(b);
	}
}

uint32_t array_buffer_get_offset(struct array_buffer *b, const uint32_t index) {
	uint32_t offset;

	assert(b != NULL);
	
	if( index >= b->count ) {
		DEBUGF("array_buffer_get_offset: index out of bounds\n");
		return (uint32_t)0xffffffff;
	}

	// what is the (max) length of 1 item
	if( b->get_length == NULL ) {
		DEBUGF("array_buffer_get_offset: get_length() function not registered\n");
		return (uint32_t)0xffffffff;
	}
	offset = b->get_length(b->max_size);

	// multiply that by the number of items before me
	if( b->sort == NULL ) { // easy, we are unsorted
		offset *= index;
	} else {
		uint32_t i;
		for(i=0; i<b->count; i++) {
			if( b->sort[i] == index )
				break;
		}
		if( i == b->count ) {
			DEBUGF("array_buffer_get_offset: index does not appeat in sorted list\n");
			return ERR_INVALID;
		}
		offset *= i; // that many items are before me
	}
	return offset;
}

uint32_t array_buffer_get_length(struct array_buffer *b) {
	uint32_t length;

	assert(b != NULL);

	// what is the (max) length of 1 item
	if( b->get_length == NULL ) {
		DEBUGF("array_buffer_get_offset: get_length() function not registered\n");
		return (uint32_t)0xffffffff;
	}
	length = b->get_length(b->max_size);

	// multiply that by the number of items
	length *= b->count;
	return length;
}

int array_buffer_write(FILE *fd, struct array_buffer *b) {
	uint32_t i;
	int rc;

	assert(b != NULL);
	assert(fd != NULL);
	
	// check if the functions exist
	if( b->write == NULL ) {
		DEBUGF("array_buffer_write: write() function not registered\n");
		return ERR_INVALID;
	}
	// if the array is in file
	// serialize and unserialize will exist, since they're checked
	// in the array_buffer_switch_to_file()
	
	if( b->fd != NULL ) {
		rewind(b->fd); // seek to the beginning
	}
	
	for(i=0; i<b->count; i++) { // for each element
		void* item;
		uint32_t j;

		// go through the sort-array and see which item should go next
		if(b->sort != NULL) {
			j = b->sort[i];
		} else j = i;
		
		// get the item in memory
		if( b->fd == NULL ) { // it already is im memory, fetch the pointer
			item = b->array[j].mem;
		} else {
			// since it's sorted, we shouldn't have to seek
			if( (rc = b->unserialize(&item, b->fd)) ) {
				DEBUGF("array_buffer_write: could not unserialize item[%u], failing...\n", i);
				return rc;
			}
		}

		if(b->pre_write != NULL && ( rc = b->pre_write(item, b->max_size) )) {
			DEBUGF("array_buffer_write: pre_write function failed, failing...\n");
			return rc;
		}

		// write item to file
		if(( rc = b->write(fd, item, b->max_size) )) {
			DEBUGF("array_buffer_write: could not write item[%u], failing...\n", i);
			return rc;
		}

		// put it back where it came from
		if( b->fd != NULL ) {
			b->destruct(item);
		}
	}

	return ERR_NONE;
}


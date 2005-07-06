#ifndef __ARRAY_BUFFER_H__
#define __ARRAY_BUFFER_H__

#include "config.h"
#include <stdio.h>
#include <stdint.h>

struct array_buffer {
	uint32_t count;			// how much items doe we have?
	
	union entry {
		void* mem;
		long file_offset;
	} *array;			// where is the data?
					// This array will always point to the same data
					// after sorting the position of the data may be canged
					// but this array will also be canged accordingly

	uint32_t *sort;			// In what order should we put the entries on disk?
	
	char* file_name;		// filename
	FILE *fd;			// file where entries are being kept. (NULL if in mem)

	int (*cmp)(const void *a, const void *b);	// compare a to b, should return:
					//   a < b ==> <0
					//   a = b ==>  0
					//   a > b ==> >0

	int  (*serialize)(FILE *fd, const void *e);	// serialize e into fd
	int  (*unserialize)(void **e, FILE *fd);	// unserialize the entry in fd

	uint32_t (*get_length)(const void *size);		// get's the length
	int  (*write)(FILE *fd, void *e, const void *size);	// write e to file

	int  (*destruct)(void *e);			// destruct object

	void *max_size;			// keep the current maximal size
	int  (*max_size_update)(void *max_size, const void *e);	// update the max_size
	int  (*max_size_destruct)(void *max_size);	// destruct the size-object

	int  (*add_item_mem)(void *e, void *s, uint32_t item);
	int  (*add_item_file)(FILE *fd, void *e, void *s, uint32_t item);

	int  (*pre_write)(void *e, void *s);	// do whatever you want, just before the entry is wrtiiten
};

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
				     );
/* This makes a new array_buffer
 *  - cmp() is the compare function used to sort: after sort cmp(item[i], item[i+1])<=0
 *  - serialize() should put the entry into the file at the current location, return 0 on success
 *  - unserialize() should read an entry from file and return the entry in memory.
 *                  return 0 on success, 1 on malloc() failures, 2 on fread() errors,
 *                  anything else on other errors
 *  - get_length() calculates the length of the entry as it will be written by write()
 *  - write() should write the entry to file in it's final format
 *  - destruct() should free all memory assigned to e (including e itself)
 *  
 *  - file_name should contain a filename that can be used as extra storage if needed
 *              if malloc()'s fail, the array is automaticaly converted to file-mode
 *              and array_buffer retries the operation.
 *              by not setting file_name=NULL malloc() failures will result in call
 *              failures
 *
 *  - max_size may be an object to record the maximal size                             \
 *  - max_size_update() will be called on each add() to update the max_size-structure   | may be NULL
 *  - max_size_destroy() should destroy the given max_size object                      /
 *
 *  - add_item_mem() add item to the entry when it is in memory    (may be NULL) 
 *  - add_item_file() add item to the serialized entry at the current file position.
 *                    the entry itself is also given in e for convenience.
 *                    If the add cannot be done in-place the function should 
 *                    - invalidate the serialized entry
 *                    - return ERR_NO_INPLACE_UPDATE
 *                    The add will be done in memory and re-added to the end of the
 *                    array    (mey be NULL)
 *             both functions must update the s-structure to reflect the maximal entry
 *
 *  - pre_write() is called right before the entry is written to disk in the write() call (may be NULL)
 * 
 * It returns that buffer on succes, NULL otherwise
 * NULL indicates a memory-allocation failure
 */

int array_buffer_destruct(struct array_buffer *b, const int free_file_name);
/* Destructs the buffer:
 *  - destructs all containing elements using the supplied destruct() function
 *  - free()'s all allocations
 *  - optionaly free()'s the file_name
 *  - free()'s b itself
 */

int array_buffer_switch_to_file(struct array_buffer *b);
/* Asks the buffer to switch to file mode
 * returns 0 on success, 1 on failure
 */

inline uint32_t array_buffer_get_next_index(struct array_buffer *b);
/* Returns the index that will be given to the next added entry
 */

int array_buffer_add(struct array_buffer *b, void *e, uint32_t *index);
/* Adds entry e to the buffer.
 * If index!=NULL *index will contain a unique number for the entry
 *
 * Returns 0 on succes, 1 otherwise
 * Once an entry is added, the caller should not use the pointer (e) anymore,
 * since array_buffer may swap the entry out to file
 */

int array_buffer_entry_update(struct array_buffer *b, const uint32_t index, uint32_t item);
/* Updates entry index with item, either in memory or in file, depending on the current
 * state of the array
 * Returns ERR_NONE on success
 *         ERR_MALLOC on malloc() failure
 *         ERR_FILE on fread(), fwrite(), fseek() problems
 */

int array_buffer_find_entry(struct array_buffer *b, const void *needle, uint32_t *index);
/* This looks for an entry that is equal to needle (i.e. that cmp(e, needle) returns 0)
 * Returns ERR_NONE on success (the entry is found)
 *         ERR_NOTFOUNF when needle was not found,
 *         ERR_MALLOC on malloc() failure
 *         ERR_FILE on fread(), fwrite() of other file() failures
 */

int array_buffer_sort(struct array_buffer *b);
/*
 */

uint32_t array_buffer_get_offset(struct array_buffer *b, const uint32_t index);
/* Returns the offset of item[index] when it would be written by the
 * array_buffer_write() call.
 * Useful to get offsets after sorting!
 */

uint32_t array_buffer_get_length(struct array_buffer *b);
/* Returns the total number of bytes array_buffer_write()
 * would write to the file
 */

int array_buffer_write(FILE *fd, struct array_buffer *b);
/* Iterate over each element and write it to file
 * returns 0 on success, 1 on failure
 */

#endif

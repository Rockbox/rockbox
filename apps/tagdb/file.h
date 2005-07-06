#ifndef __FILE_H__
#define __FILE_H__

#include "config.h"
#include <stdio.h>
#include <stdint.h>

struct file_entry {
	char* name;		// song name
	
	uint32_t hash;
	uint32_t song;		// pointer to song
	uint32_t rundb;		// pointer to rundb

	struct file_size {
		uint32_t name_len;	// must be mulitple of 4
	} size;
	unsigned char flag;	// flags
};

struct file_entry* new_file_entry(const uint32_t name_len);
/* Creates a new file_entry with the specified sizes
 * Returns a pointer to the structure on success,
 * NULL on failure
 */

int file_entry_destruct(struct file_entry *e);
/* Destructs the given file_entry and free()'s it's memory
 * returns 0 on success, 1 on failure
 */

inline int file_entry_resize(struct file_entry *e, const uint32_t name_len);
/* Change the size of the entry
 * returns 0 on succes, 1 on failure
 */

int file_entry_serialize(FILE *fd, const struct file_entry *e);
/* Serializes the entry in the file at the current position
 * returns 0 on success, 1 on failure
 */

int file_entry_unserialize(struct file_entry* *e, FILE *fd);
/* Unserializes an entry from file into a new structure
 * The address of the structure is saved into *e
 * returns 0 on success
 *         1 on malloc() failure
 *         2 on fread() failure
 */

int file_entry_write(FILE *fd, struct file_entry *e, struct file_size *s);
/* Writes the entry to file in the final form
 * returns 0 (0) on success, 1 (1) on failure
 */

inline int file_entry_compare(const struct file_entry *a, const struct file_entry *b);
/* Compares 2 entries
 * When a < b it returns <0
 *      a = b             0
 *      a > b            >0
 */

struct file_size* new_file_size();
/* Creates a new size structure
 * returns a pointer to the structure on success,
 * NULL on failure
 */

inline uint32_t file_size_get_length(const struct file_size *size);
/* Calculates the length of the entry when written by file_entry_write()
 * returns the length on success, 0xffffffff on failure
 */

inline int file_size_max(struct file_size *s, const struct file_entry *e);
/* Updates the file_size structure to contain the maximal lengths of either
 * the original entry in s, or the entry e
 * returns 0 on success, 1 on failure
 */

int file_size_destruct(struct file_size *s);
/* destructs the file_size structure
 * returns 0 on success, 1 on failure
 */

#endif

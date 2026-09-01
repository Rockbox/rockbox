#ifndef _BFILE_H_
#define _BFILE_H_

#include "bfile-internal.h"

static const int defaultPageSize   = 0x20000;  // default page buffer size, 128 kB

// NewPageReader creates a new PageReader struct with pageSize.
PageReader *NewPageReader(Handle file, s64 pageSize);

// Free all memory associated to the PageReader pointer.
void PageReader_Free(PageReader *p);

// ReadAt reads size bytes from the PageReader starting at offset.
int_error_t PageReader_ReadAt(PageReader *p, u8 *buffer, size_t size, off_t offset);

// WriteAt writes size bytes from the PageReader starting at offset.
int_error_t PageReader_WriteAt(PageReader *p, u8 *buffer, size_t size, off_t offset, bool flush_cache);

// NewPageReader creates a new PageReader struct with pageSize, for directory use.
PageReader *NewPageReaderDirectory(Handle file, s64 pageSize);

// ReadAt reads one directory entry from memory.
int_error_t PageReader_ReadDir(PageReader *p, FS_DirectoryEntry *entry);

#endif /* _B_FILE_H_ */

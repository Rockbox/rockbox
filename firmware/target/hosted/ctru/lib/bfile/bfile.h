#ifndef _BFILE_H_
#define _BFILE_H_

#include "bfile-internal.h"

static const int defaultPageSize   = 4096;     // all pages are this size
static const int defaultBufferSize = 0x800000; // default buffer size, 8 MB
static const int minPages          = 4;        // minimum total pages per file
static const int pagesPerShard     = 32;       // ideal number of pages per shard
static const int maxShards         = 128;      // maximum number of shards per file

// NewPager returns a new Pager that is backed by the provided file.
Pager* NewPager(Handle file);

// NewPagerSize returns a new Pager with a custom page size and buffer size.
// The bufferSize is the maximum amount of memory dedicated to individual
// pages. Setting pageSize and bufferSize to zero will use their defaults,
// which are 4096 and 8 MB respectively. Custom values are rounded up to the
// nearest power of 2.
Pager* NewPagerSize(Handle file, int pageSize, int bufferSize);

// ReadAt reads len(b) bytes from the File starting at byte offset off.
int_error_t PagerReadAt(Pager *f, u8 *b, size_t len_b, off_t off);

// WriteAt writes len(b) bytes to the File starting at byte offset off.
int_error_t PagerWriteAt(Pager *f, u8 *b, size_t len_b, off_t off);

// Flush writes any unwritten buffered data to the underlying file.
file_error_t PagerFlush(Pager *f);

// Truncates pager to specified length
file_error_t PagerTruncate(Pager *f, off_t length);

// Free all memory associated to a Pager file
void PagerClear(Pager *f);

#endif /* _B_FILE_H_ */

// This is a very simple code that reads blocks of data (pages) from a
// file and then simulates file reads, reading from a memory buffer.
// This is significantly faster than reading small chunks of data
// directly from a file.

#include "bfile.h"

/* Define LOGF_ENABLE to enable logf output in this file */
#if 0
#define LOGF_ENABLE
#endif
#include "logf.h"
#include "sys_fs_async_3ds.h"

/*
 * File pages start at multiples of the page capacity, so reads that move
 * back a little (tag parsers) or straddle a boundary still hit the cache.
 * The capacity never changes: a short page only means end of file.
 */

void PageReader_Invalidate(PageReader *p)
{
    p->start = 0;
    p->end = 0;
    p->size = 0;
    p->isLastPage = false;
}

// page reader implementation, offset is a multiple of the capacity
static file_error_t readPage(PageReader *p, s64 offset)
{
    u32 read_bytes = 0;
    Result res = FSFILE_Read(p->file,
                             &read_bytes,
                             offset,
                             p->buffer,
                             p->capacity);
    if (R_FAILED(res)) {
        logf("readPage: 0x%lx\n", R_DESCRIPTION(res));
        PageReader_Invalidate(p);
        return "I/O Error";
    }

    // a short page is the last page of the file
    p->isLastPage = read_bytes < p->capacity;
    p->size = read_bytes;
    p->start = offset;
    p->end = offset + read_bytes;

    logf("page: 0x%llx, 0x%llx, %lld (last_page: %s)\n", p->start, p->end, p->size, p->isLastPage == true ? "true": "false");
    return NULL;
}

PageReader* NewPageReader(Handle file, s64 pageSize)
{
    PageReader *p = (PageReader *) malloc(sizeof(PageReader));
    if (p == NULL) {
        logf("NewPageReader: memory error\n");
        return NULL;
    }

    p->buffer = malloc(sizeof(u8) * pageSize);
    if (p->buffer == NULL) {
        logf("NewPagereader: memory error (buffer)\n");
        free(p);
        return NULL;
    }

    p->capacity = pageSize;
    p->file = file;
    p->dirty = false;
    PageReader_Invalidate(p);

    // read ahead first page buffer from file
    file_error_t err =  readPage(p, 0);
    if (err != NULL) {
        PageReader_Free(p);
        return NULL;
    }

    return p;
}

void PageReader_Free(PageReader *p)
{
    if (p != NULL) {
        if (p->buffer != NULL) {
            free(p->buffer);
            p->buffer = NULL;
        }

        free(p);
    }
}

/* Returns the bytes copied in .n (never negative); .err is "io.EOF" when
 * the file ended first, or "I/O Error" */
int_error_t PageReader_ReadAt(PageReader *p, u8 *buffer, size_t size, off_t offset)
{
    // only use page_reader if buffer size is smaller than page size
    if ((s64)size > p->capacity) {
        // direct file access (fs.h)
        u32 bytes_read = 0;
        Result res = FSFILE_Read(p->file,
                                &bytes_read,
                                offset,
                                buffer,
                                size);
        if (R_FAILED(res)) {
            return (int_error_t) { 0, "I/O Error" };
        }

        // io.EOF
        if (bytes_read < size) {
            return (int_error_t) { bytes_read, "io.EOF" };
        }

        return (int_error_t) { bytes_read, NULL };
    }

    size_t done = 0;
    while (done < size) {
        s64 pos = (s64)offset + done;

        if (pos < p->start || pos >= p->end) {
            s64 page = pos - (pos % p->capacity);

            // past the end of the cached last page: end of file
            if (!(p->isLastPage && page == p->start)) {
                if (readPage(p, page) != NULL)
                    return (int_error_t) { done, "I/O Error" };
            }

            if (pos >= p->end)
                break; // end of file
        }

        size_t n = size - done;
        if ((s64)n > p->end - pos)
            n = p->end - pos;

        memcpy(buffer + done, p->buffer + (pos - p->start), n);
        done += n;
    }

    return (int_error_t) { done, done < size ? "io.EOF" : NULL };
}

int_error_t PageReader_WriteAt(PageReader *p, u8 *buffer, size_t size, off_t offset, bool flush_cache)
{
    (void)flush_cache; // the cache is kept coherent for every file
    // we will use direct file access for writing (fs.h). No FS_WRITE_FLUSH:
    // flushing every write is slow on the SD card, the file is flushed on
    // fsync() and close()
    u32 written = 0;
    Result res = FSFILE_Write(p->file,
                              &written,
                              offset,
                              buffer,
                              size,
                              0);
    if (R_FAILED(res)) {
        return (int_error_t) { 0, "I/O Error" };
    }
    p->dirty = true;

    // keep the cached page coherent: patch it when the write is inside
    // it, otherwise drop it if the write touches its range (this also
    // covers a file growing within the last page)
    s64 wstart = offset;
    s64 wend = wstart + written;
    if (written > 0) {
        if (wstart >= p->start && wend <= p->end)
            memcpy(p->buffer + (wstart - p->start), buffer, written);
        else if (wstart < p->start + p->capacity && wend > p->start)
            PageReader_Invalidate(p);
    }

    return (int_error_t) { written, NULL };
}

// page reader implementation for directory entries
static file_error_t readPageDirectory(PageReader *p)
{
    u32 entries_read = 0;
    FS_DirectoryEntry *dirEntries = (FS_DirectoryEntry *) p->buffer;
    Result result = FSDIR_Read(p->file,
                               &entries_read,
                               p->capacity,
                               dirEntries);

    if (R_FAILED(result)) {
        logf("FSDIR_Read: 0x%lx\n", R_DESCRIPTION(result));
        p->start = p->end = p->size = 0;
        return "I/O Error";
    }

    p->size = entries_read;
    p->start = 0;
    p->end = entries_read;

    // we reached directory end
    if (entries_read == 0) {
        return "io.EOF";
    }

    /* logf("page: 0x%llx, 0x%llx, %lld (last_page: %s)\n", p->start, p->end, p->size, p->isLastPage == true ? "true": "false"); */
    return NULL;
}

PageReader *NewPageReaderDirectory(Handle file, s64 pageSize)
{
    PageReader *p = (PageReader *) malloc(sizeof(PageReader));
    if (p == NULL) {
        logf("NewPageReaderDirectory: memory error\n");
        return NULL;
    }

    // round buffer size to sizeof(FS_DirectoryEntry)
    int maxDirEntries = pageSize / sizeof(FS_DirectoryEntry);

    p->buffer = malloc(sizeof(FS_DirectoryEntry) * maxDirEntries);
    if (p->buffer == NULL) {
        logf("NewPageReaderDirectory: memory error (buffer)\n");
        free(p);
        return NULL;
    }

    p->isLastPage = false;
    p->dirty = false;
    p->capacity = maxDirEntries;
    p->file = file;

    // read ahead first page buffer from file; an empty directory is fine
    file_error_t err =  readPageDirectory(p);
    if (err != NULL && strcmp(err, "io.EOF")) {
        PageReader_Free(p);
        return NULL;
    }

    return p;
}

static inline void copyEntry(PageReader *p, FS_DirectoryEntry *entry)
{
    const FS_DirectoryEntry *dirEntries = (FS_DirectoryEntry *) p->buffer;
    memcpy(entry, &dirEntries[p->start], sizeof(FS_DirectoryEntry));
    p->start ++;
}

int_error_t PageReader_ReadDir(PageReader *p, FS_DirectoryEntry *entry)
{
    // higher probability, entry is in cached page range
    if (p->start < p->end) {
        copyEntry(p, entry);
        return (int_error_t) { 1, NULL};
    }

    // less higher probability, read a new page
    file_error_t err = readPageDirectory(p);

    // end of directory?
    if (err != NULL) {
        if (!strcmp(err, "io.EOF")) {
            return (int_error_t) { 0, "io.EOF" };
        }
        else {
            return (int_error_t) { -1, "I/O Error" };
        }
    }

    copyEntry(p, entry);
    return (int_error_t) { 1, NULL};
}

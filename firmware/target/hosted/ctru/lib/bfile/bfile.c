
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

// rw mutex implementation
void sync_RWMutexInit(sync_RWMutex *m) {
    LightLock_Init(&m->shared);
    CondVar_Init(&m->reader_q);
    CondVar_Init(&m->writer_q);
    m->active_readers = 0;
    m->active_writers = 0;
    m->waiting_writers = 0;
}

static inline LightLock *unique_lock(LightLock *lk) {
    LightLock_Lock(lk);
    return lk;
}

void sync_RWMutexRLock(sync_RWMutex *m) {
    LightLock *lk = unique_lock(&m->shared);
    while (m->waiting_writers != 0) {
        CondVar_Wait(&m->reader_q, lk);
    }
    ++m->active_readers;
    LightLock_Unlock(lk);
}

void sync_RWMutexRUnlock(sync_RWMutex *m) {
    LightLock *lk = unique_lock(&m->shared);
    --m->active_readers;
    LightLock_Unlock(lk);
    CondVar_Signal(&m->writer_q);
}

void sync_RWMutexLock(sync_RWMutex *m) {
    LightLock *lk = unique_lock(&m->shared);
    ++m->waiting_writers;
    while ((m->active_readers != 0) || (m->active_writers != 0)) {
        CondVar_Wait(&m->writer_q, lk);
    }
    ++m->active_writers;
    LightLock_Unlock(lk);
}

void sync_RWMutexUnlock(sync_RWMutex *m) {
    LightLock *lk = unique_lock(&m->shared);
    --m->waiting_writers;
    --m->active_writers;
    if (m->waiting_writers > 0) {
        CondVar_Signal(&m->writer_q);
    } else {
        CondVar_Broadcast(&m->reader_q);
    }
    LightLock_Unlock(lk);
}

// page reader implementation
file_error_t readPage(PageReader *p, u64 offset)
{
    u32 read_bytes = 0;
    Result res;
    res = FSFILE_Read(p->file, 
                      &read_bytes,
                      offset,
                      p->buffer,
                      p->size);
    if (R_FAILED(res)) {
        logf("readPage: 0x%lx\n", R_DESCRIPTION(res));
        return "I/O Error";
    }

    // we are at the last page of the file
    if (read_bytes < p->size) {
        p->isLastPage = true;
    }

    p->size = read_bytes;
    p->start = offset;
    p->end = offset + p->size;

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
        return NULL;
    }

    // clear buffer data  
    memset(p->buffer, 0x0, sizeof(u8) * pageSize);

    p->start = 0;
    p->end = 0;
    p->isLastPage = false;
    p->size = pageSize;
    p->file = file;

    // read ahead first page buffer from file
    file_error_t err =  readPage(p, 0);
    if (err != NULL) {
        free(p);
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

static inline void copyPage(PageReader *p, u8 *buffer, size_t size, off_t offset)
{
    off_t off = offset - p->start;
    memcpy(buffer, p->buffer + off, size);
}

int_error_t PageReader_ReadAt(PageReader *p, u8 *buffer, size_t size, off_t offset)
{
    bool eof = false;

    // only use page_reader if buffer size is smaller than page size
    if (size > p->size) {
        // direct file access (fs.h)
        u32 bytes_read = 0;
        Result res = FSFILE_Read(p->file,
                                &bytes_read,
                                offset,
                                buffer,
                                size);
        if (R_FAILED(res)) {
            return (int_error_t) { R_DESCRIPTION(res), "I/O Error" };
        }

        // io.EOF
        if (bytes_read == 0) {
            return (int_error_t) { 0, "io.EOF" };
        }

        return (int_error_t) { bytes_read, NULL };
    }

    // higher probability, buffer is in cached page range
    if ((offset >= p->start) && ((offset + size) < p->end)) {
        copyPage(p, buffer, size, offset);
        return (int_error_t) { size, NULL };
    }

    // less higher probability, read new page at offset
    file_error_t err = readPage(p, offset);
    if (err != NULL) {
        return (int_error_t) { -1, "I/O Error" };
    }

    // we could have reached last page here so continue to
    // last page handling

    // lower probability, we are at the last page

    // trying to read past end of file
    if (offset >= p->end) {
        return (int_error_t) { 0, "io.EOF" };
    }

    // we reached the end of file so adjust buffer size to remaining bytes number
    if ((p->end - offset) < size) {
        size = p->end - offset;
        eof = true;
    }

    copyPage(p, buffer, size, offset);
    return (int_error_t) { size, eof == true ? "io.EOF" : NULL };
}

int_error_t PageReader_WriteAt(PageReader *p, u8 *buffer, size_t size, off_t offset, bool flush_cache)
{
    // we will use direct file access for writing (fs.h)
    u32 written = 0;
    Result res = FSFILE_Write(p->file,
                              &written,
                              offset,
                              buffer,
                              size,
                              FS_WRITE_FLUSH);
    if (R_FAILED(res)) {
        return (int_error_t) { -1, "I/O Error" };
    }

    if (flush_cache) {
        /* logf("PageReader_WriteAt: flush cache\n"); */
        // reload current page for O_RDWR files
        // read ahead first page buffer from file
        file_error_t err =  readPage(p, p->start);
        if (err != NULL) {
            return (int_error_t) { -1, "I/O Error" };
        }
    }

    return (int_error_t) { written, NULL };
}

// page reader implementation for directory entries
file_error_t readPageDirectory(PageReader *p)
{
    u32 entries_read = 0;
    FS_DirectoryEntry *dirEntries = (FS_DirectoryEntry *) p->buffer;
    Result result = FSDIR_Read(p->file,
                               &entries_read,
                               p->size,
                               dirEntries);

    if (R_FAILED(result)) {
        logf("FSDIR_Read: 0x%lx\n", R_DESCRIPTION(result));
        return "I/O Error";
    }

    // we reached directory end
    if (entries_read == 0) {
        return "io.EOF";
    }

    if (entries_read < p->size) {
         p->isLastPage = true;
    }

    p->size = entries_read;
    p->start = 0;
    p->end = p->size;

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
        return NULL;
    }

    // clear buffer data  
    memset(p->buffer, 0x0, sizeof(FS_DirectoryEntry) * maxDirEntries);

    p->start = 0;
    p->end = 0;
    p->isLastPage = false;
    p->size = maxDirEntries;
    p->file = file;

    // read ahead first page buffer from file
    file_error_t err =  readPageDirectory(p);
    if (err != NULL) {
        free(p);
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

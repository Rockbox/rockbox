/* 
 *  This code is based on bfile.go by Josh Baker.
 *  Converted to C code by Mauricio G.
 *  Released under the MIT License.
 */

/*  IMPORTANT: this code only works for O_RDONLY and O_RDWR files. */

#include "bfile.h"

/* note: for ease of reading and ease of comparing go code
   with c implementation, function names are similar to the
   go version */

/* note2: sync_RWMutex calls have been moved to rockbox sys_file
   implementation.  To use as standalone code please uncomment those calls. */

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

void s_init(shard* s);

void s_push(shard* s, page* p) {
    s->head->next->prev = p;
    p->next = s->head->next;
    p->prev = s->head;
    s->head->next = p;
}

void s_pop(shard* s, page* p) {
    p->prev->next = p->next;
    p->next->prev = p->prev;
}

void s_bump(shard* s, page* p) {
    s_pop(s, p);
    s_push(s, p);
}

/* page_pair_t destructor */
/* page_pair_t type is defined by cmap_declare(page, s64, struct page*) */
/* struct {
        s64 key;
        struct page* value;
   } page_pair_t;
*/
void page_pair_free(void* pair_ptr) {
    if (pair_ptr) {
        page_pair_t *pair = (page_pair_t*) pair_ptr;
        struct page *p = pair->value;
        if (p != nil)  {
            if (p->data != nil) {
              free(p->data);
            }
            free(p);
        }
    }
}

/* shard destructor */
void s_free(void* s_ptr) {
    if (s_ptr) {
        shard *s = (shard*) s_ptr;
        if (s->pages != nil)  {
            cmap_set_elem_destructor(s->pages, page_pair_free);
            cmap_clear(page, s->pages);
            cmap_clear(bool, s->dirty);
            free(s->head);
            free(s->tail);
        }
    }
}

Pager* NewPager(Handle file) {
    return NewPagerSize(file, 0, 0);
}

Pager* NewPagerSize(Handle file, int pageSize, int bufferSize) {
    if (pageSize <= 0) {
        pageSize = defaultPageSize;
    } else if ((pageSize & 4095) != 0) {
        // must be a power of two
        int x = 1;
        while (x < pageSize) {
            x *= 2;
        }
        pageSize = x;
    }

    if (bufferSize <= 0) {
        bufferSize = defaultBufferSize;
    } else if (bufferSize < pageSize) {
        bufferSize = pageSize;
    }

    Pager* f = (Pager*) malloc(sizeof(Pager));
    f->file = file;
    f->size = -1;
    f->pgsize = (s64) pageSize;

    /* sync_RWMutexInit(&f->mu); */

    // calculate the max number of pages across all shards
    s64 pgmax = (s64) bufferSize / f->pgsize;
    if (pgmax < minPages) {
        pgmax = minPages;
    }

    // calculate how many shards are needed, power of 2
    s64 nshards = (s64) ceil((double) pgmax / (double) pagesPerShard);
    if (nshards > maxShards) {
        nshards = maxShards;
    }
    s64 x = 1;
    while (x < nshards) {
        x *= 2;
    }
    nshards = x;

    // calculate the max number of pages per shard
    f->pgmax = (s64) floor((double) pgmax / (double) nshards);
    cslice_make(f->shards, nshards, (shard) { 0 });

    // initialize sync mutex
    size_t i;
    for (i = 0; i < cslice_len(f->shards); i++) {
        sync_MutexInit(&f->shards[i].mu);
    }
    return f;
}

static int_error_t read_at(Handle file, u8 *data, size_t data_len, off_t off)
{
    u32 read_bytes = 0;
    if (R_FAILED(FSFILE_Read(file, &read_bytes, (u64) off, data, (u32) data_len))) {
        return (int_error_t) { -1, "I/O error" }; 
    }

    /* io.EOF */
    if (read_bytes == 0) {
        return (int_error_t) { 0, "io.EOF" }; 
    }

    return (int_error_t) { (int) read_bytes, nil };
}

static int_error_t write_at(Handle file, u8 *data, size_t data_len, off_t off)
{
    u32 written = 0;
    if (R_FAILED(FSFILE_Write(file, &written, (u64) off, data, (u32) data_len, FS_WRITE_FLUSH))) {
        return (int_error_t) { -1, "I/O error" }; 
    }

    /* I/O error */
    if ((written == 0) || (written < (u32) data_len)) {
        return (int_error_t) { -1, "I/O error" }; 
    }

    return (int_error_t) { (int) written, nil };
}

static stat_error_t file_stat(Handle file)
{
    u64 size = 0;
    struct stat fi = { 0 };
    if (R_FAILED(FSFILE_GetSize(file, &size))) {
        fi.st_size = 0;
        return (stat_error_t) { fi, "I/O error" };
    }

    fi.st_size = (off_t) size;
    return (stat_error_t) { fi, nil };
}

void s_init(shard* s)
{
    if (s->pages == nil)  {
        s->pages = cmap_make(/*s64*/page);
        s->dirty = cmap_make(/*s64*/bool);
        s->head = (page*) malloc(sizeof(page));
        s->tail = (page*) malloc(sizeof(page));
        s->head->next = s->tail;
        s->tail->prev = s->head;
    }
}

file_error_t f_write(Pager* f, page* p) {
    s64 off = p->num * f->pgsize;
    s64 end = f->pgsize;
    if ((off + end) > f->size) {
        end = f->size - off;
    }
    int_error_t __err = write_at(f->file, p->data, end, off);
    if (__err.err != nil) {
        return __err.err;
    }
    return nil;
}

file_error_t f_read(Pager* f, page* p) {
    int_error_t __err = read_at(f->file, p->data, f->pgsize, p->num * f->pgsize);
    if ((__err.err != nil) && strcmp(__err.err, "io.EOF")) {
        return "I/O error";
    }

    return nil;
}

const char* f_incrSize(Pager* f, s64 end, bool write)
{
#define defer(m)                \
    sync_RWMutexUnlock(&f->mu); \
    sync_RWMutexRLock(&f->mu);

    /* sync_RWMutexRUnlock(&f->mu);
    sync_RWMutexLock(&f->mu); */

    if (f->size == -1) {
        stat_error_t fi_err = file_stat(f->file);
        if (fi_err.err != nil) {
            /* defer(&f->mu); */
            return nil;
        }
        f->size = fi_err.fi.st_size;
    }
    if (write && (end > f->size)) {
        f->size = end;
    }

    /* defer(&f->mu); */
    return nil;
}

int_error_t f_pio(Pager *f, u8 *b, size_t len_b, s64 pnum, s64 pstart, s64 pend, bool write);
int_error_t f_io(Pager *f, u8 *b, size_t len_b, s64 off, bool write) {
    if (f == nil) {
        return (int_error_t) { 0, "invalid argument" };
    }
    bool eof = false;
    s64 start = off, end = off + len_b;
    if (start < 0) {
        return (int_error_t) { 0, "negative offset" };
    }

    // Check the upper bounds of the input to the known file size.
    // Increase the file size if needed.
    /* sync_RWMutexRLock(&f->mu); */
    if (end > f->size) {
        file_error_t err = f_incrSize(f, end, write);
        if (err != nil) {
            /* sync_RWMutexRUnlock(&f->mu); */
            return (int_error_t) { 0, err };
        }
        if (!write && (end > f->size)) {
            end = f->size;
            if ((end - start) < 0) {
                end = start;
            }
            eof = true;
            len_b = end-start; /* b = b[:end-start] */
        }
    }
    /* sync_RWMutexRUnlock(&f->mu); */

    // Perform the page I/O.
    int total = 0;
    while (len_b > 0) {
        s64 pnum = off / f->pgsize;
        s64 pstart = off & (f->pgsize - 1);
        s64 pend = pstart + (s64) len_b;
        if (pend > f->pgsize) {
            pend = f->pgsize;
        }

        int_error_t result = f_pio(f, b, pend - pstart, pnum, pstart, pend, write);
        if (result.err != nil) {
            return (int_error_t) { total, result.err };
        }

        off += (s64) result.n;
        total += result.n;
        b = &b[result.n]; len_b -= result.n; /* b = b[n:] */
    }
    if (eof) {
        return (int_error_t) { total, "io.EOF" };
    }

    return (int_error_t) { total, nil };
}

int_error_t f_pio(Pager *f, u8 *b, size_t len_b, s64 pnum, s64 pstart, s64 pend, bool write) {
    /* printf("pio(%p, %d, %lld, %lld, %lld, %s)\n", b, len_b, pnum, pstart, pend, write == true ? "true" : "false"); */
    shard *s = &f->shards[pnum & (s64) (cslice_len(f->shards) - 1)];
    sync_MutexLock(&s->mu);
    s_init(s);
    page *p = cmap_get_ptr(page, s->pages, pnum);
    if (p == nil) {
        // Page does not exist in memory.
        // Acquire a new one.
        if (cmap_len(s->pages) == f->pgmax) {
            // The buffer is at capacity.
            // Evict lru page and hang on to it.
            p = s->tail->prev;
            s_pop(s, p);
            cmap_delete(page, s->pages, p->num);
            if (cmap_get(bool, s->dirty, p->num)) {
                // dirty page. flush it now
                file_error_t err = f_write(f, p);
                if (err != nil) {
                    sync_MutexUnlock(&s->mu);
                    return (int_error_t) { 0, err };
                }
                cmap_delete(bool, s->dirty, p->num);
            }
            // Clear the previous page memory for partial page writes for
            // pages that are being partially written to.
            if (write && ((pend - pstart) < f->pgsize)) {
                memset(p->data, 0, f->pgsize);
            }
        } else {
            // Allocate an entirely new page.
            p = (page *) malloc(sizeof(page));
            p->data = (u8 *) malloc(f->pgsize);
        }
        p->num = pnum;
        // Read contents of page from file for all read operations, and
	// partial write operations. Ignore for full page writes.
        if (!write || ((pend-pstart) < f->pgsize)) {
            file_error_t err = f_read(f, p);
            if (err != nil) {
                sync_MutexUnlock(&s->mu);
                return (int_error_t) { 0, err };
            }
        }
        // Add the newly acquired page to the list.
        cmap_set(page, s->pages, p->num, p);
        s_push(s, p);
    } else {
        // Bump the page to the front of the list.
        s_bump(s, p);
    }
    if (write) {
        memcpy(p->data + pstart, b, pend - pstart);
        cmap_set(bool, s->dirty, pnum, true);
    } else {
        memcpy(b, p->data + pstart, pend - pstart);
    }
    sync_MutexUnlock(&s->mu);
    return (int_error_t) { len_b, nil };
}

// Flush writes any unwritten buffered data to the underlying file.
file_error_t PagerFlush(Pager *f) {
    if (f == nil) {
        return "invalid argument";
    }

    /* sync_RWMutexLock(&f->mu); */
    for (size_t i = 0; i < cslice_len(f->shards); i++) {
        cmap_iterator(bool) pnum;
        if (f->shards[i].dirty != nil) {
            for (pnum = cmap_begin(f->shards[i].dirty);
                pnum != cmap_end(f->shards[i].dirty); pnum++) {
                if (pnum->value == true) {
                    page *p = cmap_get_ptr(page, f->shards[i].pages, pnum->key);
                    if (p != nil) {
                        file_error_t err = f_write(f, p);
                        if (err != nil) {
                            /* sync_RWMutexUnlock(&f->mu); */
                            return err;
                        }
                    }
                    cmap_set(bool, f->shards[i].dirty, pnum->key, false);
                }
            }
        }
    }
    /* sync_RWMutexUnlock(&f->mu); */
    return nil;
}

// ReadAt reads len(b) bytes from the File starting at byte offset off.
int_error_t PagerReadAt(Pager *f, u8 *b, size_t len_b, off_t off) {
    return f_io(f, b, len_b, off, false);
}

// WriteAt writes len(b) bytes to the File starting at byte offset off.
int_error_t PagerWriteAt(Pager *f, u8 *b, size_t len_b, off_t off) {
    return f_io(f, b, len_b, off, true);
}

file_error_t PagerTruncate(Pager *f, off_t length) {
    if (f == nil) {
        return "invalid argument";
    }

    /* flush unwritten changes to disk */
    PagerFlush(f);

    /* sync_RWMutexRLock(&f->mu); */
    /* set new file size */
    Handle handle = f->file;
    Result res = FSFILE_SetSize(handle, (u64) length);
    if (R_FAILED(res)) {
        return "I/O error";
    }
    /* sync_RWMutexRUnlock(&f->mu); */
    
    /* FIXME: truncate only required pages.  Remove all for now */
    PagerClear(f);
    f = NewPager(handle);
    return nil;
}

void PagerClear(Pager *f) {
    if (f) {
        cslice_set_elem_destructor(f->shards, s_free);
        cslice_clear(f->shards);
        free(f);
    }
}


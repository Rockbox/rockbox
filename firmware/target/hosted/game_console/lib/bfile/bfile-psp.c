// Mauricio Garrido, 2026.

// This is a very simple code that reads blocks of data (pages) from a
// file and then simulates file reads, reading from a memory buffer.
// This is significantly faster than reading small chunks of data
// directly from a file.

// important: written only for rockbox use.

#include "bfile.h"

/* Define LOGF_ENABLE to enable logf output in this file */
#if 0
#define LOGF_ENABLE
#endif
#include "logf.h"

// page reader implementation
file_error_t readPage(PageReader *reader, s64 offset)
{
    Page *p = &reader->p;

    // non-contiguous page
    if (offset != p->end) {
        logf("readPage: performing lseek to offset 0x%llx.\n", offset);
        int pos = sceIoLseek32(reader->fd, offset, SEEK_SET);
        if (pos < 0) {
            logf("warning: sceIoLseek32 error (0x%x).\n", pos);
        }

        reader->last_offset = pos;
    }

    int bytes_read = sceIoRead(reader->fd, p->buffer, p->size);
    if (bytes_read < 0) {
        logf("readPage: sceIoRead error (0x%x)\n", bytes_read);
        return "I/O Error";
    }

    // if no error, it means we are at the last page of the file
    if (bytes_read < p->size) {
        p->isLastPage = true;
    }

    p->size = bytes_read;
    p->start = offset;
    p->end = offset + p->size;

    /* logf("page: 0x%llx, 0x%llx, %lld (last_page: %s)\n", p->start, p->end, p->size, p->isLastPage == true ? "true": "false"); */
    return NULL;
}

PageReader* NewPageReader(SceUID fd, s64 pageSize)
{
    PageReader *reader = (PageReader *) malloc(sizeof(PageReader));
    if (reader == NULL) {
        logf("NewPageReader: memory error\n");
        return NULL;
    }

    reader->fd = fd;
    reader->last_offset = 0;

    // setup default page
    Page *p = &reader->p;

    p->buffer = malloc(sizeof(u8) * pageSize);
    if (p->buffer == NULL) {
        logf("NewPagereader: memory error (page buffer)\n");
        return NULL;
    }

    // clear buffer data
    memset(p->buffer, 0x0, sizeof(u8) * pageSize);

    p->start = 0;
    p->end = 0;
    p->isLastPage = false;
    p->size = pageSize;

    // for psp, we need to make sure the file descriptor
    // points to the start of the file
    sceIoLseek32(fd, 0, SEEK_SET);

    // read ahead first page buffer from file
    file_error_t err =  readPage(reader, 0);
    if (err != NULL) {
        free(p->buffer);
        free(reader);
        return NULL;
    }

    return reader;
}

void PageReader_Free(PageReader *reader)
{
    if (reader != NULL) {
        Page *p = &reader->p;
        if (p->buffer != NULL) {
            free(p->buffer);
            p->buffer = NULL;
        }

        free(reader);
    }
}

static inline void copyPage(Page *p, u8 *buffer, size_t size, off_t offset)
{
    off_t off = offset - p->start;
    memcpy(buffer, p->buffer + off, size);
}

int_error_t PageReader_ReadAt(PageReader *reader, u8 *buffer, size_t size, off_t offset)
{
    bool eof = false;
    Page *p = &reader->p;

    // only use page_reader if buffer size is smaller than page size
    if (size > p->size) {
        if (reader->last_offset != offset) {
            logf("PageReader_ReadAt: reading at arbitrary offset.");
            int pos = sceIoLseek32(reader->fd, offset, SEEK_SET);
            if (pos < 0) {
                logf("warning: sceIoLseek32 error (0x%x).\n", pos);
            }

            reader->last_offset = pos;
        }

        // direct file access (pspiofilemgr.h)
        int bytes_read = sceIoRead(reader->fd, buffer, size);
        if (bytes_read < 0) {
            return (int_error_t) { bytes_read, "I/O Error" };
        }

        reader->last_offset += bytes_read;

        // io.EOF
        if (bytes_read == 0) {
            return (int_error_t) { 0, "io.EOF" };
        }

        return (int_error_t) { bytes_read, NULL };
    }

    // higher probability, buffer is in cached page range
    if ((offset >= p->start) && ((offset + size) < p->end)) {
        if (reader->fd == 1)
            printf("cached page\n");
        copyPage(p, buffer, size, offset);
        return (int_error_t) { size, NULL };
    }

    if (reader->fd == 1)
        printf("non cached page\n");

    // less higher probability, read new page at offset
    file_error_t err = readPage(reader, offset);
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

    reader->last_offset += size;

    copyPage(p, buffer, size, offset);
    return (int_error_t) { size, eof == true ? "io.EOF" : NULL };
}

int_error_t PageReader_WriteAt(PageReader *reader, u8 *buffer, size_t size, off_t offset, bool flush_cache)
{
    if (reader->last_offset != offset) {
        logf("PageReader_WriteAt: writting at arbitrary offset.");
        int pos = sceIoLseek32(reader->fd, offset, SEEK_SET);
        if (pos < 0) {
            logf("warning: sceIoLseek32 error (0x%x).\n", pos);
        }

        reader->last_offset = pos;
    }

    // we will use direct file access for writing (fs.h)
    int written = sceIoWrite(reader->fd, buffer, size);
    if (written < 0) {
        return (int_error_t) { -1, "I/O Error" };
    }

    reader->last_offset += written;

    if (flush_cache) {
        /* logf("PageReader_WriteAt: flush cache\n"); */
        // reload current page for O_RDWR files
        // read ahead first page buffer from file
        file_error_t err =  readPage(reader, reader->p.start);
        if (err != NULL) {
            return (int_error_t) { -1, "I/O Error" };
        }
    }

    return (int_error_t) { written, NULL };
}

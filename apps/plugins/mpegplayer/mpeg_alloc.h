#ifndef MPEG_ALLOC_H
#define MPEG_ALLOC_H

/* returns the remaining mpeg2 buffer and it's size */
void * mpeg2_get_buf(size_t *size);
void *mpeg_malloc(size_t size, mpeg2_alloc_t reason);
/* Grabs all the buffer available sans margin */
void *mpeg_malloc_all(size_t *size_out, mpeg2_alloc_t reason);
/* Initializes the malloc buffer with the given base buffer */
bool mpeg_alloc_init(unsigned char *buf, size_t mallocsize);

#endif /* MPEG_ALLOC_H */

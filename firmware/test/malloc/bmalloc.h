int bmalloc_add_pool(void *start, size_t size);
void bmalloc_status(void);

void *bmalloc(size_t size);
void bfree(void *ptr);


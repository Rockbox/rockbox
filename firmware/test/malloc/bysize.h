void bmalloc_remove_chunksize(void *data);
void bmalloc_insert_bysize(char *data, size_t size);
char *bmalloc_obtainbysize( size_t size);
#ifdef DEBUG
void bmalloc_print_sizes(void);
#endif

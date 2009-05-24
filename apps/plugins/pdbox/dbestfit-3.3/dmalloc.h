void *dmalloc(size_t);
void dfree(void *);
void *drealloc(void *, size_t);

#define malloc(x)    dmalloc(x)
#define free(x)      dfree(x)
#define realloc(x,y) drealloc(x,y)

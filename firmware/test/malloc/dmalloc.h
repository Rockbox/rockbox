
void *dmalloc(size_t);
void dfree(void *);
void *drealloc(void *, size_t);

#define malloc(x)    dmalloc(x)
#define free(x)      dfree(x)
#define realloc(x,y) drealloc(x,y)
#define calloc(x,y)  dcalloc(x,y)


/* use this to intialize the internals of the dmalloc engine */
void dmalloc_initialize(void);

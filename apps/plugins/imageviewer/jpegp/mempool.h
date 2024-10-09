#include <inttypes.h>

extern void init_mem_pool(const unsigned char *buf, const ssize_t buf_size);
extern void *malloc(size_t size);
extern void *calloc(size_t nelem, size_t elem_size);
extern ssize_t freeze_mem_pool(void);
extern void clear_mem_pool(void);

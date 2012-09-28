#include <stddef.h>

struct mem_info_t {
    void *dram;
    size_t dram_size;
    size_t dram_used;
    void *iram;
    size_t iram_size;
    size_t iram_used;
};

void *elf_open(const char *filename, struct mem_info_t *m);

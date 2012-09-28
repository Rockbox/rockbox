#include <stddef.h>

struct mem_info_t {
    void *dram;
    void *dram_img;
    size_t dram_size;
    size_t dram_used;

    void *iram;
    void *iram_img;
    size_t iram_size;
    size_t iram_used;

    uint32_t uncache_base;
};

void *elf_open(const char *filename, struct mem_info_t *m);

#include "config.h"
#include <stddef.h>

#if defined(CPU_ARM) || defined(CPU_MIPS) || defined(CPU_SH)
#define ELF_USE_REL
#elif defined(CPU_COLDFIRE)
#define ELF_USE_RELA
#else
#error "unknown CPU type"
#endif

struct mem_info_t {
    void *dram;
    size_t dram_size;
    size_t dram_runtime_use;
    size_t dram_load_use;
    void *iram;
    size_t iram_size;
    size_t iram_runtime_use;
};

void *elf_open(const char *filename, struct mem_info_t *m);

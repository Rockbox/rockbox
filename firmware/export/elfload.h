#include "config.h"
#include <stddef.h>
#include "elf.h"

#if defined(CPU_ARM) || defined(CPU_MIPS)
#define ELF_USE_REL
#elif defined(CPU_COLDFIRE) || defined(CPU_SH)
#define ELF_USE_RELA
#else
#error "unknown CPU type"
#endif

#define HI16_TAB_ENTRIES 16
#define VENEER_SLOTS 16

#define MAX_ELF_SECTIONS 32
#define COMMON_IDX 32

struct mem_info_t {
    void *addr;
    size_t size;
    size_t runtime_use;
    size_t load_use;
};

enum mem_segment_t {
    DRAM,
    IRAM,
    UNKNOWN_SEGMENT
};

/* ARM specific */
enum veneer_type_t {
    ARM_ARM_LONG,
    ARM_THM_LONG,
    THM_ARM_LONG,
    THM_THM_LONG
};

struct veneer_t
{
    Elf32_Word *where;
    Elf32_Addr target;
    enum mem_segment_t mem_segment;
    enum veneer_type_t type;
};

/* MIPS specific */
struct r_mips_hi16 {
    Elf_Addr *where;
    Elf_Addr symval;
};

struct load_info_t {
    struct mem_info_t mem[2];
    char *sect_addr[MAX_ELF_SECTIONS+1];
    Elf32_Sym *symtab;
    size_t symtab_size;
#if defined(CPU_ARM)
    struct veneer_t veneer_tab[VENEER_SLOTS];
    int veneer_idx;
#elif defined(CPU_MIPS)
    struct r_mips_hi16 r_mips_hi16_tab[HI16_TAB_ENTRIES];
    int r_hi16_idx;
#endif
};

void *elf_open(const char *filename, struct load_info_t *l);

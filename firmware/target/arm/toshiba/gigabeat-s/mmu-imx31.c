#include <string.h>
#include "s3c2440.h"
#include "mmu-imx31.h"
#include "panic.h"

static void enable_mmu(void);
static void set_ttb(void);
static void set_page_tables(void);
static void map_section(unsigned int pa, unsigned int va, int mb, int cache_flags);

void memory_init(void) {
}

void set_ttb() {
}

void set_page_tables() {
}

void map_section(unsigned int pa, unsigned int va, int mb, int cache_flags) {
    (void)pa;
    (void)va;
    (void)mb;
    (void)cache_flags;
}

static void enable_mmu(void) {
}

void invalidate_dcache_range(const void *base, unsigned int size) {
    (void)base;
    (void)size;
}

void clean_dcache_range(const void *base, unsigned int size) {
    (void)base;
    (void)size;
}

void dump_dcache_range(const void *base, unsigned int size) {
    (void)base;
    (void)size;
}
/* Cleans entire DCache */
void clean_dcache(void)
{
}


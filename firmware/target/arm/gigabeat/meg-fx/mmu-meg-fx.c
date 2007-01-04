#include <string.h>
#include "s3c2440.h"
#include "mmu-meg-fx.h"

void map_memory(void);
static void enable_mmu(void);
static void set_ttb(void);
static void set_page_tables(void);
static void map_section(unsigned int pa, unsigned int va, int mb, int cache_flags);

#define SECTION_ADDRESS_MASK (-1 << 20)
#define CACHE_ALL (1 << 3 | 1 << 2 )
#define CACHE_NONE 0
#define BUFFERED (1 << 2)
#define MB (1 << 20)

void map_memory(void) {
    set_ttb();
    set_page_tables();
    enable_mmu();
}

unsigned int* ttb_base;
const int ttb_size = 4096;

void set_ttb() {
    int i;
    int* ttbPtr;
    int domain_access;

    /* must be 16Kb (0x4000) aligned */
    ttb_base = (int*)0x31F00000;
    for (i=0; i<ttb_size; i++,ttbPtr++)
    ttbPtr = 0;
    asm volatile("mcr p15, 0, %0, c2, c0, 0" : : "r" (ttb_base));

    /* set domain D0 to "client" permission access */

    domain_access = 3;
    asm volatile("mcr p15, 0, %0, c3, c0, 0" : : "r" (domain_access));

}

void set_page_tables() {

    map_section(0, 0, 0x1000, CACHE_NONE);

    map_section(0x30000000, 0, 32, CACHE_NONE); /* map RAM to 0 */

    map_section(0x30000000, 0, 30, CACHE_ALL); /* cache the first 30 MB or RAM */
    map_section(0x31E00000, 0x31E00000, 1, BUFFERED); /* enable buffered writing for the framebuffer */
}

void map_section(unsigned int pa, unsigned int va, int mb, int cache_flags) {
    unsigned int* ttbPtr;
    int i;
    int section_no;

    section_no = va >> 20; /* sections are 1Mb size */
    ttbPtr = ttb_base + section_no;
    pa &= SECTION_ADDRESS_MASK; /* align to 1Mb */
    for(i=0; i<mb; i++, pa += MB) {
    *(ttbPtr + i) =
        pa |
        1 << 10 |    /* superuser - r/w, user - no access */
        0 << 5 |    /* domain 0th */
        1 << 4 |    /* should be "1" */
        cache_flags |
        1 << 1;    /* Section signature */
    }
}

static void enable_mmu(void) {
    asm volatile("mov r0, #0\n"
        "mcr p15, 0, r0, c8, c7, 0\n" /* invalidate TLB */

        "mcr p15, 0, r0, c7, c7,0\n" /* invalidate both icache and dcache */

        "mrc p15, 0, r0, c1, c0, 0\n"
        "orr r0, r0, #1<<0\n" /* enable mmu bit, icache and dcache */
        "orr r0, r0, #1<<2\n" /* enable dcache */
        "orr r0, r0, #1<<12\n" /* enable icache */
        "mcr p15, 0, r0, c1, c0, 0" : : : "r0");
    asm volatile("nop \n nop \n nop \n nop");
}

/* Invalidate DCache for this range  */
/* Will do write back */
void invalidate_dcache_range(const void *base, unsigned int size) {
    unsigned int addr = (int) base;
    unsigned int end = addr+size;
    asm volatile(
    "bic %0, %0, #31 \n"
"inv_start: \n"
    "mcr p15, 0, %0, c7, c14, 1 \n" /* Clean and invalidate this line */
    "add %0, %0, #32 \n"
    "cmp %0, %1 \n"
    "blo inv_start \n"
    "mov %0, #0\n"
    "mcr p15,0,%0,c7,c10,4\n"    /* Drain write buffer */
        : : "r" (addr), "r" (end));
}

/* clean DCache for this range  */
/* forces DCache writeback for the specified range */
void clean_dcache_range(const void *base, unsigned int size) {
    unsigned int addr = (int) base;
    unsigned int end = addr+size;
    asm volatile(
    "bic %0, %0, #31 \n"
"clean_start: \n"
    "mcr p15, 0, %0, c7, c10, 1 \n" /* Clean this line */
    "add %0, %0, #32 \n"
    "cmp %0, %1 \n"
    "blo clean_start \n"
    "mov %0, #0\n"
    "mcr p15,0,%0,c7,c10,4 \n"    /* Drain write buffer */
        : : "r" (addr), "r" (end));
}

/* Dump DCache for this range  */
/* Will *NOT* do write back */
void dump_dcache_range(const void *base, unsigned int size) {
    unsigned int addr = (int) base;
    unsigned int end = addr+size;
    asm volatile(
    "tst %0, #31 \n"
    "bic %0, %0, #31 \n"
    "mcr p15, 0, %0, c7, c14, 1 \n" /* Clean and invalidate this line */
    "add %0, %0, #32 \n"
    "tst %1, #31 \n"
    "bic %1, %1, #31 \n"
    "mcrne p15, 0, %1, c7, c14, 1 \n" /* Clean and invalidate this line, if not cache aligned */
    "cmp %0, %1 \n"
    "beq dump_end \n"
"dump_start: \n"
    "mcr p15, 0, %0, c7, c6, 1 \n" /* Invalidate this line */
    "add %0, %0, #32 \n"
    "cmp %0, %1 \n"
    "bne dump_start \n"
"dump_end: \n"
    "mcr p15,0,%0,c7,c10,4 \n"    /* Drain write buffer */
        : : "r" (addr), "r" (end));
}
/* Cleans entire DCache */
void clean_dcache(void)
{
    unsigned int seg, index, addr;

    /* @@@ This is straight from the manual.  It needs to be optimized. */
    for(seg = 0; seg <= 7; seg++) {
        for(index = 0; index <= 63; index++) {
            addr = (seg << 5) | (index << 26);
            asm volatile(
                "mcr p15, 0, %0, c7, c10, 2 \n" /* Clean this entry by index */
                : : "r" (addr));
        }
    }
}


#define CONFIG_X1000
/* jztool loads us where it loads the Rockbox bootloader, after the SPL has
 * set up the DDR. Below that is free (Rockbox keeps its exception vectors
 * there), and so is the DRAM past DRAM_SIZE. */
#define DRAM_ORIG       0x80004000
#define DRAM_SIZE       (1024 * 1024 - 0x4000)
#define CPU_MIPS
#define STACK_SIZE      0x1000
#define DCACHE_SIZE     0x4000 /* 16 kB */
#define DCACHE_LINE_SIZE    0x20   /* 32 B */
#define ICACHE_SIZE     0x4000 /* 16 kB */
#define ICACHE_LINE_SIZE    0x20   /* 32 B */
/* we need to flush caches before executing */
#define CONFIG_FLUSH_CACHES

/* something provides define
 * #define mips 1
 * which breaks paths badly
 */
#undef mips

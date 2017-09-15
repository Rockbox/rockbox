#define CONFIG_ATJ213X
#define IRAM_ORIG       0x94040000 /* KSEG1 cached unmapped */
#define IRAM_SIZE       0x18000
#define DRAM_ORIG       0x80000000 /* KSEG1 cached unmapped */
#define DRAM_SIZE       0x800000
#define DCACHE_SIZE     0x4000     /* 16 kB */
#define DCACHE_LINE_SIZE    0x10   /* 16 B */
#define ICACHE_SIZE     0x4000     /* 16 kB */
#define ICACHE_LINE_SIZE    0x10   /* 61 B */
/* we need to flush caches before executing */
#define CONFIG_FLUSH_CACHES

#define CPU_MIPS

/* something provides define
 * #define mips 1
 * which breaks paths badly
 */
#undef mips

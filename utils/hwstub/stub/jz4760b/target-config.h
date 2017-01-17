#define CONFIG_JZ4760B
#define TCSM0_ORIG      0xf4000000
#define TCSM0_SIZE      0x4000
#define TCSM0_UNCACHED_ADDRESS  0xb32b0000
#define CPU_MIPS
#define STACK_SIZE      0x300
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

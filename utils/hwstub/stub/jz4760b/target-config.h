#define CONFIG_JZ4760B
#define IRAM_ORIG       0x80000000 /* KSEG1 cached unmapped */
#define IRAM_SIZE       0x2000
#define DRAM_ORIG       0x80000000 /* KSEG1 cached unmapped */
#define DRAM_SIZE       0x100000
#define CPU_MIPS
#define STACK_SIZE      0x200

/* something provides define
 * #define mips 1
 * which breaks paths badly
 */
#undef mips

#define CONFIG_JZ4760B
#define TCSM0_ORIG      0xf4000000
#define TCSM0_SIZE      0x4000
#define CPU_MIPS
#define STACK_SIZE      0x300

/* something provides define
 * #define mips 1
 * which breaks paths badly
 */
#undef mips

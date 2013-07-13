#define CONFIG_STMP
#define IRAM_ORIG       0
#define IRAM_SIZE       0x8000
#define DRAM_ORIG       0x40000000
#define DRAM_SIZE       (MEMORYSIZE * 0x100000)
#define CPU_ARM
#define ARM_ARCH        5
#define USB_BASE            0x80080000
#define USB_NUM_ENDPOINTS   2
#define CONFIG_PP
#define IRAM_ORIG       0x40000000
#define IRAM_SIZE       0x20000
#define DRAM_ORIG       0x10f00000
#define DRAM_SIZE       (MEMORYSIZE * 0x100000)
#define CPU_ARM
#define ARM_ARCH        5
#define USB_BASE            0xc5000000
#define USB_NUM_ENDPOINTS   2
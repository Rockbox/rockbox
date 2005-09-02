/* define this if you have recording possibility */
#define HAVE_RECORDING 1

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* LCD dimensions */
#define LCD_WIDTH  112
#define LCD_HEIGHT 64
#define LCD_DEPTH  1

/* define this if you have an Ondio style 6-key keyboard */
#define CONFIG_KEYPAD ONDIO_PAD

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x8000

/* Define this if you have an FM Radio */
#define CONFIG_TUNER (S1A0903X01 | TEA5767) /* to be decided at runtime */
#define CONFIG_TUNER_XTAL 13000000

#ifndef SIMULATOR

/* Define this if you have a SH7034 */
#define CONFIG_CPU SH7034

/* Define this if you have a MAS3587F */
#define CONFIG_CODEC MAS3587F

/* Define this to the CPU frequency */
#define CPU_FREQ      12000000

/* Define this for different I2C pinout */
#define CONFIG_I2C I2C_ONDIO

/* Type of mobile power */
#define CONFIG_BATTERY BATT_3AAA

/* Battery scale factor (average from 3 Ondios) */
#define BATTERY_SCALE_FACTOR 4735

/* Define this if you control power on PB5 (instead of the OFF button) */
#define HAVE_POWEROFF_ON_PB5

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 20

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 6

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 24

/* Define this if the tuner is switched on by software */
#define HAVE_TUNER_PWR_CTRL

/* How to detect USB */
#define USB_FMRECORDERSTYLE 1 /* like FM, on AN1 */

/* How to enable USB */
#define USB_ENABLE_ONDIOSTYLE 1 /* with PA5 */

/* The start address index for ROM builds */
/* #define ROM_START 0x16010 for behind original Archos */
#define ROM_START 0x7010 /* for behind BootBox */

/* Define this if the display is mounted upside down */
#define HAVE_DISPLAY_FLIPPED

/* Define this for different I2C pinout */
#define HAVE_ONDIO_I2C

/* Define this for different ADC channel assignment */
#define HAVE_ONDIO_ADC

/* Define this for MMC support instead of ATA harddisk */
#define HAVE_MMC

/* Define this to support mounting FAT16 partitions */
#define HAVE_FAT16SUPPORT

/* Define this if the MAS SIBI line can be controlled via PB8 */
#define HAVE_MAS_SIBI_CONTROL

/* define this if more than one device/partition can be used */
#define HAVE_MULTIVOLUME

/* define this if media can be exchanged on the fly */
#define HAVE_HOTSWAP

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

#define CONFIG_LCD LCD_SSD1815

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

#define BOOTFILE_EXT "ajz"
#define BOOTFILE "ajbrec." BOOTFILE_EXT

#endif /* SIMULATOR */

/*
 * This config file is for iriver iFP-799
 */
#define TARGET_TREE

#define IRIVER_IFP7XX_SERIES 1

#define MODEL_NAME   "iriver iFP-799"

/* For Rolo and boot loader */
#define MODEL_NUMBER 6

/* define this if you have recording possibility */
/*#define HAVE_RECORDING*/

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
/* #define HAVE_LCD_COLOR */

/* define this if you would like tagcache to build on this target */
/* #define HAVE_TAGCACHE */

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 64
#define LCD_DEPTH  1

#define LCD_PIXELFORMAT VERTICAL_PACKING

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_DARKCOLOR       0x000000
#define LCD_BRIGHTCOLOR     0x5e6854
#define LCD_BL_DARKCOLOR    0x000000
#define LCD_BL_BRIGHTCOLOR  0x3ca0e6

#define CONFIG_KEYPAD IRIVER_IFP7XX_PAD

#define CONFIG_STORAGE STORAGE_NAND

#define CONFIG_NAND NAND_IFP7XX

#define HAVE_FAT16SUPPORT

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x38000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x10000

/* Define this if you have the WM8975 audio codec */
/* #define HAVE_WM8975 */

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

#define HAVE_LCD_CONTRAST

#define MIN_CONTRAST_SETTING        5
#define MAX_CONTRAST_SETTING        63
#define DEFAULT_CONTRAST_SETTING    40

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

#define BATTERY_CAPACITY_DEFAULT 1000 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 500  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2800 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  2    /* Alkalines or NiMH */

/* define this if the unit should not shut down on low battery. */
#define NO_LOW_BATTERY_SHUTDOWN

#ifndef SIMULATOR

/* Define this if you have a Philips PNX0101 */
#define CONFIG_CPU PNX0101

/* Define this if you want to use the PNX0101 i2c interface */
#define CONFIG_I2C I2C_PNX0101

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

/* Define this to the CPU frequency */
#define CPU_FREQ      48000000

#define CONFIG_LCD LCD_IFP7XX

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_ISP1582

#define HAVE_GDB_API

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "iriver"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define IBSS_ATTR_VOICE_STACK
#define ICODE_ATTR_TREMOR_NOT_MDCT
#define ICODE_ATTR_TREMOR_MDCT
#define ICODE_ATTR_FLAC
#define IBSS_ATTR_FLAC_DECODED0
#define ICONST_ATTR_MPA_HUFFMAN
#define IBSS_ATTR_MPC_SAMPLE_BUF
#define ICODE_ATTR_ALAC
#define IBSS_ATTR_SHORTEN_DECODED0

#endif

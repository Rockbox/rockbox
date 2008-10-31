/*
 * This config file is for the Sansa C100 series
 */
#define TARGET_TREE /* this target is using the target tree system */

#define MODEL_NAME "Sandisk Sansa c100 series"

/* For Rolo and boot loader */
#define MODEL_NUMBER 30

/* define this if you have recording possibility */
//#define HAVE_RECORDING

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
//#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_SPDIF)

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 64
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565   /*rgb565*/

/*#define LCD_PIXELFORMAT VERTICAL_PACKING*/

/* define this to indicate your device's keypad */
#define CONFIG_KEYPAD SANSA_C100_PAD

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_TCC77X

/* define this if you have RTC RAM available for settings */
//#define HAVE_RTC_RAM

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x8000

#define AB_REPEAT_ENABLE 1

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this if you have the TLV320 audio codec */
#define HAVE_TLV320

/* TLV320 has no tone controls, so we use the software ones */
#define HAVE_SW_TONE_CONTROLS

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#define CONFIG_I2C I2C_TCC77X

#define BATTERY_CAPACITY_DEFAULT 540 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 540 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 540 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

/* define this if the unit should not shut down on low battery. */
#define NO_LOW_BATTERY_SHUTDOWN

#ifndef SIMULATOR

/* Define this if you have a TCC770 */
#define CONFIG_CPU TCC770

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Define this to the CPU frequency */
#define CPU_FREQ      120000000

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 4

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 6

/* The start address index for ROM builds */
/* #define ROM_START 0x11010 for behind original Archos */
#define ROM_START 0x7010 /* for behind BootBox */        

/* Software controlled LED */
#define CONFIG_LED LED_VIRTUAL

#define CONFIG_LCD LCD_S6B33B2 /* Not sure about this... same as C200? - MarcGuay */

#define BOOTFILE_EXT "c100"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/"

#define CONFIG_STORAGE STORAGE_NAND

#define CONFIG_NAND NAND_TCC

#endif /* SIMULATOR */

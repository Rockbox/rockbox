/*
 * This config file is for iriver iFP-799
 */
#define IRIVER_IFP7XX_SERIES 1

/* For Rolo and boot loader */
#define MODEL_NUMBER 6

/* define this if you have recording possibility */
/*#define HAVE_RECORDING 1*/

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have a colour LCD */
/* #define HAVE_LCD_COLOR 1 */

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 64
#define LCD_DEPTH  1

#define CONFIG_KEYPAD IRIVER_IFP7XX_PAD

#define CONFIG_FLASH FLASH_IFP7XX

#define HAVE_FAT16SUPPORT

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x30000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x20000

/* Define this if you have the WM8975 audio codec */
/* #define HAVE_WM8975 */

#define HAVE_FLASH_DISK

#ifndef SIMULATOR

/* Define this if you have a Philips PNX0101 */
#define CONFIG_CPU PNX0101

/* Define this if you want to use the PNX0101 i2c interface */
#define CONFIG_I2C I2C_PNX0101

/* Type of mobile power */
//#define CONFIG_BATTERY BATT_LIPOL1300

#define BATTERY_SCALE_FACTOR 16665 /* FIX: this value is picked at random */

/* Define this if the platform can charge batteries */
//#define HAVE_CHARGING 1

/* define this if the hardware can be powered off while charging */
//#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_IRIVER_IFP7XX /* port controlled */

/* Define this to the CPU frequency */
#define CPU_FREQ      30000000

#define CONFIG_LCD LCD_IFP7XX

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_ISP1582

#define HAVE_GDB_API

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Define this if you have adjustable CPU frequency */
//#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT "iriver"
#define BOOTFILE "rockbox." BOOTFILE_EXT

#endif

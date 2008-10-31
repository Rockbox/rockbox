/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

#define MODEL_NAME   "Ondio SP"

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  112
#define LCD_HEIGHT 64
#define LCD_DEPTH  1

#define LCD_PIXELFORMAT VERTICAL_PACKING

/* define this if you have an Ondio style 6-key keyboard */
#define CONFIG_KEYPAD ONDIO_PAD

#define AB_REPEAT_ENABLE 1
#define ACTION_WPSAB_SINGLE ACTION_WPS_BROWSE

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x8000

/* Define this if you have a MAS3539F */
#define CONFIG_CODEC MAS3539F

/* Enable this if you have done the backlight mod */
//#define HAVE_BACKLIGHT

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

/* define this if more than one device/partition can be used */
#define HAVE_MULTIVOLUME

#define BATTERY_CAPACITY_DEFAULT 1000 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 500  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 1500 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  2    /* Alkalines or NiMH */

/* define this if the unit should not shut down on low battery. */
#define NO_LOW_BATTERY_SHUTDOWN

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

#ifndef SIMULATOR

/* Define this if you have a SH7034 */
#define CONFIG_CPU SH7034

/* Define this to the CPU frequency */
#define CPU_FREQ      12000000

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 20

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 6

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 24

/* The start address index for ROM builds */
/* #define ROM_START 0x12010 for behind original Archos */
#define ROM_START 0x7010 /* for behind BootBox */

/* Define this if the display is mounted upside down */
#define HAVE_DISPLAY_FLIPPED

/* Define this for different I2C pinout */
#define CONFIG_I2C I2C_ONDIO

/* Define this for different ADC channel assignment */
#define HAVE_ONDIO_ADC

/* Define this for MMC support instead of ATA harddisk */
#define CONFIG_STORAGE STORAGE_MMC

/* Define this to support mounting FAT16 partitions */
#define HAVE_FAT16SUPPORT

/* Define this if the MAS SIBI line can be controlled via PB8 */
#define HAVE_MAS_SIBI_CONTROL

/* define this if media can be exchanged on the fly */
#define HAVE_HOTSWAP

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

#define CONFIG_LCD LCD_SSD1815

#define BOOTFILE_EXT "ajz"
#define BOOTFILE "ajbrec." BOOTFILE_EXT
#define BOOTDIR "/"

#endif /* SIMULATOR */

#define HAVE_LCD_CONTRAST

#define MIN_CONTRAST_SETTING        5
#define MAX_CONTRAST_SETTING        63


/*
 * This config file is for the Cowon iAudio D2
 */
#define TARGET_TREE /* this target is using the target tree system */

/* For Rolo and boot loader */
#define MODEL_NUMBER 24

/* Produce a dual-boot bootloader.bin for mktccboot */
#define TCCBOOT

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
/* #define HAVE_LCD_FLIP */

/* define this if you can invert the colours on your LCD */
/* #define HAVE_LCD_INVERT */

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN
/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
//#define HAVE_TAGCACHE

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

/* Some (2Gb?) D2s seem to be FAT16 formatted */
#define HAVE_FAT16SUPPORT

/* LCD dimensions */
#define LCD_WIDTH  320
#define LCD_HEIGHT 240
#define LCD_DEPTH  16
#define LCD_PIXELFORMAT 565

/* define this if you have LCD enable function */
#define HAVE_LCD_ENABLE

/* define this to indicate your device's keypad */
#define CONFIG_KEYPAD COWOND2_PAD

/* define this if you have a real-time clock */
//#define CONFIG_RTC RTC_TCC780X

/* define this if you have RTC RAM available for settings */
//#define HAVE_RTC_RAM

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Reduce Tremor's ICODE usage */
#define ICODE_ATTR_TREMOR_NOT_MDCT

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE 1

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* The D2 uses a WM8985 codec */
#define HAVE_WM8985

/* There is no hardware tone control */
#define HAVE_SW_TONE_CONTROLS

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

/* Enable LCD brightness control */
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      14
#define DEFAULT_BRIGHTNESS_SETTING  8

#define CONFIG_I2C I2C_TCC780X

#define BATTERY_CAPACITY_DEFAULT 1500 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1500 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

/* define this if the unit should not shut down on low battery. */
/* TODO: this is temporary until battery monitoring implemented */
#define NO_LOW_BATTERY_SHUTDOWN

#ifndef SIMULATOR

/* Define this if you have a TCC7801 */
#define CONFIG_CPU TCC7801

/* Define this if you have ATA power-off control */
#define HAVE_ATA_POWER_OFF

/* Define this to the CPU frequency */
#define CPU_FREQ 48000000

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* Software controlled LED */
#define CONFIG_LED LED_VIRTUAL

#define CONFIG_LCD LCD_COWOND2

#define BOOTFILE_EXT "iaudio"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/"

#endif /* SIMULATOR */

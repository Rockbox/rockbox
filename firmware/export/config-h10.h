/*
 * This config file is for the iriver H10 20Gb
 */

#define TARGET_TREE /* this target is using the target tree system */

/* For Rolo and boot loader */
#define MODEL_NUMBER 13

/* define this if you have recording possibility */
/*#define HAVE_RECORDING 1*/ /* TODO: add support for this */

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP 1

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR 1

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* LCD dimensions */
#define LCD_WIDTH  160
#define LCD_HEIGHT 128
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* #define IRAM_LCDFRAMEBUFFER IDATA_ATTR *//* put the lcd frame buffer in IRAM */

#define CONFIG_KEYPAD IRIVER_H10_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#ifndef BOOTLOADER
#define CONFIG_RTC RTC_E8564
#endif

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8975 audio codec */
#define HAVE_WM8731

#define AB_REPEAT_ENABLE 1

/* FM Tuner */
/*#define CONFIG_TUNER TEA5767
#define CONFIG_TUNER_XTAL  32768 *//* TODO: what is this? */

/* Define this for LCD backlight available */
#define CONFIG_BACKLIGHT BL_H10 /* TODO: figure this out, probably not necessary
                                   because of 'target' stuff */

#define BATTERY_CAPACITY_DEFAULT 1550 /* default battery capacity
                                        TODO: check this, probably different
                                        for different models too */

#ifndef SIMULATOR

/* Define this if you have a PortalPlayer PP5020 */
#define CONFIG_CPU PP5020

/* Define this if you want to use the PP5020 i2c interface */
#define CONFIG_I2C I2C_PP5020

/* Type of mobile power */
#define CONFIG_BATTERY BATT_LPCS355385
#define BATTERY_CAPACITY_MIN 1500  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 1600 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 10   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */
#define BATTERY_SCALE_FACTOR 5865

/* Hardware controlled charging? FIXME */
//#define CONFIG_CHARGING CHARGING_SIMPLE

/* define this if the hardware can be powered off while charging */
/* TODO: should this be set for the H10? */
//#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* Define this to the CPU frequency */
#define CPU_FREQ      75000000

/* Type of LCD */
#define CONFIG_LCD LCD_H10_20GB

/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST

#define MIN_CONTRAST_SETTING        0
#define MAX_CONTRAST_SETTING        30
#define DEFAULT_CONTRAST_SETTING    14 /* Match boot contrast */

/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
 * should be defined as well.
 * We can currently put the lcd to sleep but it won't wake up properly */
/*#define HAVE_LCD_SLEEP*/

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* #define USB_IPODSTYLE */

/* define this if the unit can be powered or charged via USB */
/*#define HAVE_USB_POWER*/

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Define this if you have adjustable CPU frequency */
/*#define HAVE_ADJUSTABLE_CPU_FREQ*/

#define BOOTFILE_EXT "h10"
#define BOOTFILE "rockbox." BOOTFILE_EXT

#endif

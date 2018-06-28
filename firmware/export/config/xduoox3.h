/*
 * This config file is for xDuoo X3
 */

#define MODEL_NAME   "xDuoo X3"
#define MODEL_NUMBER 96

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC  0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* Support FAT16 for SD cards <= 2GB */
#define HAVE_FAT16SUPPORT

/* ChinaChip NAND FTL */
#define CONFIG_NAND NAND_CC

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 64
/* sqrt(128^2 + 64^2) / 1.0 = 143.1 */
#define LCD_DPI 143
#define LCD_DEPTH  1

#define LCD_PIXELFORMAT VERTICAL_PACKING
#define HAVE_NEGATIVE_LCD /* bright on dark */

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_DARKCOLOR        0x000000
#define LCD_BRIGHTCOLOR      0x000000
#define LCD_BL_DARKCOLOR     0x000000
#define LCD_BL_BRIGHTCOLOR   0x0de2e5

/* define this if you have LCD enable function */
#define HAVE_LCD_ENABLE

#ifndef BOOTLOADER
/* Define this if your LCD can be put to sleep.
 * HAVE_LCD_ENABLE should be defined as well. */
#define HAVE_LCD_SLEEP
#define HAVE_LCD_SLEEP_SETTING
#endif

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you can invert the pixels */
#define HAVE_LCD_INVERT

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

#define IRAM_LCDFRAMEBUFFER IDATA_ATTR /* put the lcd frame buffer in IRAM */

/* Define this if you can detect headphones */
#define HAVE_HEADPHONE_DETECTION

#define CONFIG_KEYPAD XDUOO_X3_PAD

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

#ifndef BOOTLOADER
/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_JZ4760
#endif

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x100000

#define HAVE_CS4398
#define CODEC_SLAVE

/* has no tone controls, so we use the software ones */
#define HAVE_SW_TONE_CONTROLS

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS       SAMPR_CAP_ALL

#define AB_REPEAT_ENABLE

#define BATTERY_CAPACITY_DEFAULT 2000 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 500      /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2000     /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 100      /* capacity increment */
#define BATTERY_TYPES_COUNT  1        /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* There is only USB charging */
#define HAVE_USB_POWER

#define CFG_EXTAL 12000000    /* EXT clock: 12 Mhz */

/* define this if the flash memory uses the SecureDigital Memory Card protocol */
#define CONFIG_STORAGE (STORAGE_SD /* | STORAGE_NAND */)

#define HAVE_MULTIDRIVE
#define NUM_DRIVES 2

/* Define this if media can be exchanged on the fly */
#ifndef BOOTLOADER
#define HAVE_HOTSWAP
#define HAVE_HOTSWAP_STORAGE_AS_MAIN
#endif

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

/** Non-simulator section **/
#ifndef SIMULATOR

/* Define this if you have a Ingenic JZ4760B */
#define CONFIG_CPU JZ4760B

/* Define this to the CPU frequency */
#define CPU_FREQ 492000000    /* CPU clock: 492 MHz */

/* Define this if you want to use the JZ47XX i2c interface */
#define CONFIG_I2C I2C_JZ47XX

#define NEED_ADC_CLOSE 1

/* define this if the hardware can be powered off while charging */
/* #define HAVE_POWEROFF_WHILE_CHARGING */

/* Type of LCD */
#define CONFIG_LCD LCD_XDUOOX3

/* USB On-the-go */
#define CONFIG_USBOTG     USBOTG_JZ4760

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK

/* Connect by events, not by tick polling */
#define USB_STATUS_BY_EVENT

#define USB_VENDOR_ID 0x0525
#define USB_PRODUCT_ID 0xA4A5

#define USB_NUM_ENDPOINTS  3
#define USB_DEVBSS_ATTR    IBSS_ATTR

#define BOOTFILE_EXT "x3"
#define BOOTFILE     "rockbox." BOOTFILE_EXT
#define BOOTDIR      "/.rockbox"

#define INCLUDE_TIMEOUT_API

#define ICODE_ATTR_TREMOR_NOT_MDCT

#endif /* SIMULATOR */

/** Port-specific settings **/

#define CONFIG_SDRAM_START 0x80004000

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING          1
#define MAX_BRIGHTNESS_SETTING          16
#define DEFAULT_BRIGHTNESS_SETTING      16 /* "full brightness" */

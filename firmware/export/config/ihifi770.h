/*
 * This config file is for IHIFI 770
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 98

#define MODEL_NAME   "IHIFI 770"

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
/* #define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_FM) */

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_96 | SAMPR_CAP_48 | SAMPR_CAP_44 | \
                         SAMPR_CAP_32 | SAMPR_CAP_24 | SAMPR_CAP_22 | \
                         SAMPR_CAP_16 | SAMPR_CAP_12 | SAMPR_CAP_11 | SAMPR_CAP_8)

#define HAVE_WM8740
#define CODEC_SLAVE

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you can flip your LCD */
/* #define HAVE_LCD_FLIP */

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you can invert the colours on your LCD */
/* #define HAVE_LCD_INVERT */

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

#define CONFIG_STORAGE (STORAGE_SD | STORAGE_NAND)

#define CONFIG_NAND NAND_RK27XX
#define HAVE_SW_TONE_CONTROLS

#define HAVE_HOTSWAP

#define NUM_DRIVES 1
#define SECTOR_SIZE 512

/* for small(ish) SD cards */
#define HAVE_FAT16SUPPORT

/* LCD dimensions */
#define LCD_WIDTH  320
#define LCD_HEIGHT 240
/* sqrt(320^2 + 240^2) / 2.4 = 166.7 */
#define LCD_DPI 167
#define LCD_DEPTH  16   /* pseudo 262.144 colors */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* Define this if the LCD can shut down */
/* #define HAVE_LCD_SHUTDOWN */

/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
#ifndef BOOTLOADER
/* TODO: #define HAVE_LCD_SLEEP */
/* TODO: #define HAVE_LCD_SLEEP_SETTING */
#endif

#define CONFIG_KEYPAD IHIFI_770_PAD

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* Define this if a programmable hotkey is mapped */
/* #define HAVE_HOTKEY */

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
/* #define CONFIG_RTC RTC_NANO2G */

/* Define if the device can wake from an RTC alarm */
/* #define HAVE_RTC_ALARM */

/* Define the type of audio codec */
/*#define HAVE_RK27XX_CODEC */

/* #define HAVE_PCM_DMA_ADDRESS */

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define MIN_BRIGHTNESS_SETTING      0
#define MAX_BRIGHTNESS_SETTING      31
#define DEFAULT_BRIGHTNESS_SETTING  31
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_HW_REG

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define BATTERY_CAPACITY_DEFAULT 1050 /* default battery capacity */
#define BATTERY_CAPACITY_MIN      500 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX     1050 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC       10 /* capacity increment */
#define BATTERY_TYPES_COUNT         1 /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define current usage levels */
/* TODO: #define CURRENT_NORMAL
 * TODO: #define CURRENT_BACKLIGHT  23
 */

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define this if your LCD can set contrast */
/* #define HAVE_LCD_CONTRAST */

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define STORAGE_NEEDS_ALIGN

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/** Non-simulator section **/
#ifndef SIMULATOR

/* The exact type of CPU */
#define CONFIG_CPU RK27XX

/* Define this to the CPU frequency */
#define CPU_FREQ        200000000

/* I2C interface */
#define CONFIG_I2C I2C_RK27XX

/* define this if the hardware can be powered off while charging */
/* #define HAVE_POWEROFF_WHILE_CHARGING */

/* Type of LCD */
#define CONFIG_LCD LCD_IHIFI770

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_RK27XX

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK

#define USB_VENDOR_ID 0x071b
#define USB_PRODUCT_ID 0x3202
#define HAVE_BOOTLOADER_USB_MODE

#define RKW_FORMAT
#define BOOTFILE_EXT "rkw"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

/* disabled for now */
#undef HAVE_HOTSWAP

#endif /* SIMULATOR */

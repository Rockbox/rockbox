/*
 * This config file is for HiFi E.T. MA9 reference design
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 84

#define MODEL_NAME   "HiFi E.T. MA9C"

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_96 | SAMPR_CAP_48 | SAMPR_CAP_44 | \
                         SAMPR_CAP_32 | SAMPR_CAP_24 | SAMPR_CAP_22 | \
                         SAMPR_CAP_16 | SAMPR_CAP_12 | SAMPR_CAP_11 | SAMPR_CAP_8)

#define HAVE_DF1704_CODEC

#define CODEC_SLAVE



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

/* commented for now */
/* #define HAVE_HOTSWAP */

#define NUM_DRIVES 2
#define SECTOR_SIZE 512

/* for small(ish) SD cards */
#define HAVE_FAT16SUPPORT

/* LCD dimensions */
#define LCD_WIDTH  320
#define LCD_HEIGHT 240
/* sqrt(320^2 + 240^2) / 2.4 = 169.5 */
#define LCD_DPI 169
#define LCD_DEPTH  16   /* pseudo 262.144 colors */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE

#define CONFIG_KEYPAD MA_PAD

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT




#define CONFIG_LCD LCD_ILI9342C

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define MIN_BRIGHTNESS_SETTING      0
#define MAX_BRIGHTNESS_SETTING      31
#define DEFAULT_BRIGHTNESS_SETTING   31
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_HW_REG

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* TODO: Figure out real values */
#define BATTERY_CAPACITY_DEFAULT 600 /* default battery capacity */
#define BATTERY_CAPACITY_MIN     300 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX     600 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC      10 /* capacity increment */
#define BATTERY_TYPES_COUNT        1 /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Hardware controlled charging with monitoring */
#define CONFIG_CHARGING CHARGING_MONITOR

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_RK27XX

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK

#define USB_VENDOR_ID 0x071b
#define USB_PRODUCT_ID 0x3202
#define HAVE_BOOTLOADER_USB_MODE

/* Define this if your LCD can set contrast */
/* #define HAVE_LCD_CONTRAST */

/* The exact type of CPU */
#define CONFIG_CPU RK27XX

/* I2C interface */
#define CONFIG_I2C I2C_RK27XX

/* Define this to the CPU frequency */
#define CPU_FREQ        200000000

/* define this if the hardware can be powered off while charging */
/* #define HAVE_POWEROFF_WHILE_CHARGING */

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

#define RKW_FORMAT
#define BOOTFILE_EXT "rkw"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

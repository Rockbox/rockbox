/*
 * This config file is for the Philips GoGear HDD16x0/HDD63x0
 */

#define TARGET_TREE /* this target is using the target tree system */

/* For Rolo and boot loader */
#define MODEL_NUMBER 31
#define MODEL_NAME   "Philips GoGear HDD16x0"

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

/* define this if you have recording possibility */
/* #define HAVE_RECORDING */

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_LINEIN | SRC_CAP_FMRADIO)

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_48 | \
                         SAMPR_CAP_44 | SAMPR_CAP_32 | SAMPR_CAP_8)

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS  (SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_48 | \
                         SAMPR_CAP_44 | SAMPR_CAP_32 | SAMPR_CAP_8)

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 128
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565  /* rgb565 byte-swapped */

#ifndef BOOTLOADER
/* Define this if your LCD can be enabled/disabled */
/* #define HAVE_LCD_ENABLE */

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
 * should be defined as well.
 * We can currently put the lcd to sleep but it won't wake up properly */
/* #define HAVE_LCD_SLEEP */
/* #define HAVE_LCD_SLEEP_SETTING */
#endif

/* define this if you can flip your LCD */
/* #define HAVE_LCD_FLIP */

/* define this if you can invert the colours on your LCD */
/* #define HAVE_LCD_INVERT */

/* #define IRAM_LCDFRAMEBUFFER IDATA_ATTR *//* put the lcd frame buffer in IRAM */

#define CONFIG_KEYPAD PHILIPS_HDD1630_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock. The HDD16x0 has a PCF8563 RTC,
   but it's register compatible with the E8564. */
#ifndef BOOTLOADER
#define CONFIG_RTC RTC_E8564
#define HAVE_RTC_ALARM
#endif

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8731 audio codec */
#define HAVE_WM8731

/* WM8731 has no tone controls, so we use the software ones */
#define HAVE_SW_TONE_CONTROLS

/* TODO: #define AB_REPEAT_ENABLE 1 */

/* FM Tuner */
/* #define CONFIG_TUNER TEA5767 */
/* #define CONFIG_TUNER_XTAL  32768 */

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
/* #define HAVE_BACKLIGHT_BRIGHTNESS */

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING     12
#define DEFAULT_BRIGHTNESS_SETTING  6

/* define this if you have a light associated with the buttons */
/* #define HAVE_BUTTON_LIGHT */

#define BATTERY_CAPACITY_DEFAULT 1550 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1500  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

/* Hardware controlled charging */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

#ifndef SIMULATOR

/* Define this if you have a PortalPlayer PP5020 */
#define CONFIG_CPU PP5022

/* Define this if you want to use the PP5020 i2c interface */
#define CONFIG_I2C I2C_PP5020

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* The size of the flash ROM */
#define FLASH_SIZE 0x100000

/* Define this to the CPU frequency */
#define CPU_FREQ      75000000

/* Type of LCD */
#define CONFIG_LCD LCD_HDD1630

/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST

#define MIN_CONTRAST_SETTING        0
#define MAX_CONTRAST_SETTING        30
#define DEFAULT_CONTRAST_SETTING    14 /* Match boot contrast */

/* We're able to shut off power to the HDD */
#define HAVE_ATA_POWER_OFF

/* Offset ( in the firmware file's header ) to the file CRC and data. These are
   only used when loading the old format rockbox.e200 file */
#define FIRMWARE_OFFSET_FILE_CRC    0x0
#define FIRMWARE_OFFSET_FILE_DATA   0x8

/* #define USB_IPODSTYLE */

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_ARC

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0x0471
#define USB_PRODUCT_ID 0x014C

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define MI4_FORMAT
#define BOOTFILE_EXT    "mi4"
#define BOOTFILE        "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define ICODE_ATTR_TREMOR_NOT_MDCT

#endif

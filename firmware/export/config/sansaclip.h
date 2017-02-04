/*
 * This config file is for the Sandisk Sansa Clip
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 40
#define MODEL_NAME   "Sandisk Sansa Clip"
#define FIRMWARE_OFFSET_FILE_DATA 8
#define FIRMWARE_OFFSET_FILE_CRC 0

#define HW_SAMPR_CAPS       SAMPR_CAP_ALL

/* define this if you have recording possibility */
#define HAVE_RECORDING

#define REC_SAMPR_CAPS      (SAMPR_CAP_48 | SAMPR_CAP_44 | SAMPR_CAP_32 | \
                             SAMPR_CAP_24 | SAMPR_CAP_22 | SAMPR_CAP_16 | \
                             SAMPR_CAP_12 | SAMPR_CAP_11 | SAMPR_CAP_8)

/* because the samplerates don't match at each point, we must be able to
 * tell PCM which set of rates to use. not needed if recording rates are
 * a simple subset of playback rates and are equal values. */
#define CONFIG_SAMPR_TYPES

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_FMRADIO)

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a light associated with the buttons */
#define HAVE_BUTTON_LIGHT

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 64
/* sqrt(128^2 + 64^2) / 1.0 = 143.1 */
#define LCD_DPI 143
#define LCD_DEPTH  1

#define LCD_PIXELFORMAT VERTICAL_PACKING
#define HAVE_NEGATIVE_LCD /* bright on dark */
#define HAVE_LCD_SPLIT    /* split display */

/* Display colours, for screenshots and sim (0xRRGGBB) */
#define LCD_DARKCOLOR        0x000000
#define LCD_BRIGHTCOLOR      0x000000
#define LCD_BL_DARKCOLOR     0x000000
#define LCD_BL_BRIGHTCOLOR   0x0de2e5

#define LCD_DARKCOLOR_2       0x000000
#define LCD_BRIGHTCOLOR_2     0x000000
#define LCD_BL_DARKCOLOR_2    0x000000
#define LCD_BL_BRIGHTCOLOR_2  0xffe60f

#define LCD_SPLIT_POS    16
#define LCD_SPLIT_LINES  2

/* define this if you have LCD enable function */
#define HAVE_LCD_ENABLE

#ifndef BOOTLOADER
/* Define this if your LCD can be put to sleep.
 * HAVE_LCD_ENABLE should be defined as well. */
//#define HAVE_LCD_SLEEP
//#define HAVE_LCD_SLEEP_SETTING
#endif

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you can invert the pixels */
#define HAVE_LCD_INVERT

/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST

#define MIN_CONTRAST_SETTING        0
#define MAX_CONTRAST_SETTING        50
#define DEFAULT_CONTRAST_SETTING    30

#define IRAM_LCDFRAMEBUFFER IBSS_ATTR /* put the lcd frame buffer in IRAM */

#define CONFIG_KEYPAD SANSA_CLIP_PAD

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC
/* There is no hardware tone control */
#define HAVE_SW_TONE_CONTROLS

/* We're working on the assumption that the AS3525 has something
   similar to the AS3514 for audio codec etc */
#define HAVE_AS3514

/* define this if you have a real-time clock */
#ifndef BOOTLOADER
#define CONFIG_RTC RTC_AS3514
#endif

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

#define HAVE_FAT16SUPPORT

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x48000  /* in IRAM */

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x10000

#define AB_REPEAT_ENABLE

/* FM Tuner */
#define CONFIG_TUNER SI4700 /* in fact SI4702 but let's hope it's compatible */
//#define HAVE_TUNER_PWR_CTRL

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

/* define this if the flash memory uses the SecureDigital Memory Card protocol */
#define CONFIG_STORAGE STORAGE_SD

#define BATTERY_CAPACITY_DEFAULT 330    /* default battery capacity */
#define BATTERY_CAPACITY_MIN 330        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 330        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0          /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Charging implemented in a target-specific algorithm */
#define CONFIG_CHARGING CHARGING_TARGET

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define this if you have a AMS AS3525 SoC */
#define CONFIG_CPU AS3525

/* Define how much SD sectors are reserved for OF */
#define AMS_OF_SIZE 0x5000

/* Define this if you want to use the AS3525 i2c interface */
#define CONFIG_I2C I2C_AS3525

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* define current usage levels (based on battery bench) */
#define CURRENT_NORMAL     37
#define CURRENT_BACKLIGHT  13
#define CURRENT_RECORD     CURRENT_NORMAL

/* maximum charging current */
#define CURRENT_MAX_CHG   150

/* Define this to the CPU frequency */
#define CPU_FREQ      248000000

/* Type of LCD */
#define CONFIG_LCD LCD_SSD1303

#define CONFIG_USBOTG USBOTG_AS3525

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK
//#define USB_HANDLED_BY_OF
#define USB_VENDOR_ID 0x0781
#define USB_PRODUCT_ID 0x7433
#define HAVE_USB_HID_MOUSE
#define HAVE_BOOTLOADER_USB_MODE

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT    "sansa"
#define BOOTFILE        "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define ICODE_ATTR_TREMOR_NOT_MDCT

#define INCLUDE_TIMEOUT_API

/** Port-specific settings **/

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING     12
#define DEFAULT_BRIGHTNESS_SETTING  6

/* Default recording levels */
#define DEFAULT_REC_MIC_GAIN    23
#define DEFAULT_REC_LEFT_GAIN   23
#define DEFAULT_REC_RIGHT_GAIN  23

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

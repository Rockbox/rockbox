/*
 * This config file is for the Sandisk Sansa c200v2
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 44
#define MODEL_NAME   "Sandisk Sansa c200v2 series"

#define HW_SAMPR_CAPS       SAMPR_CAP_ALL_96

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




/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you have a light associated with the buttons */
#define HAVE_BUTTON_LIGHT

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  132
#define LCD_HEIGHT 80
/* sqrt(132^2 + 80^2) / 1.4 = 110.3 */
#define LCD_DPI 110
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* define this if you have LCD enable function */
#define HAVE_LCD_ENABLE

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
/* TODO: #define HAVE_LCD_SLEEP */
/* TODO: #define HAVE_LCD_SLEEP_SETTING <= optional */

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST

#define MIN_CONTRAST_SETTING        0
#define MAX_CONTRAST_SETTING        127
#define DEFAULT_CONTRAST_SETTING    85

#if 0   /* not enough room for a 16 bits framebuffer */
#define IRAM_LCDFRAMEBUFFER IDATA_ATTR /* put the lcd frame buffer in IRAM */
#endif

#define CONFIG_KEYPAD SANSA_C200_PAD

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST




/* There is no hardware tone control */
#define HAVE_SW_TONE_CONTROLS

/* The AS3525 has a built-in AS3514 (or a newer version) */
#define HAVE_AS3514

/* define this if you have a real-time clock */
#ifndef BOOTLOADER
#define CONFIG_RTC RTC_AS3514
#endif

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Some Sansa E200s seem to be FAT16 formatted */
#define HAVE_FAT16SUPPORT

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x48000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x14000

#define AB_REPEAT_ENABLE

/* FM Tuner - suspected to be the SI4702 */
#define CONFIG_TUNER SI4700
/* #define HAVE_TUNER_PWR_CTRL */

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

/* define this if the flash memory uses the SecureDigital Memory Card protocol */
#define CONFIG_STORAGE STORAGE_SD

#define BATTERY_CAPACITY_DEFAULT 530    /* default battery capacity */
#define BATTERY_CAPACITY_MIN 530        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 530        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0          /* capacity increment */


#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Charging implemented in a target-specific algorithm */
#define CONFIG_CHARGING CHARGING_TARGET

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define this if you have a PortalPlayer PP5024 */
#define CONFIG_CPU AS3525

/* Define how much SD sectors are reserved for OF */
#define AMS_OF_SIZE 0x5000

/* Define this if you want to use the PP5024 i2c interface */
#define CONFIG_I2C I2C_AS3525

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

#define CURRENT_NORMAL     50
#define CURRENT_BACKLIGHT  25
#define CURRENT_RECORD     CURRENT_NORMAL

/* maximum charging current */
#define CURRENT_MAX_CHG   200

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* Define this to the CPU frequency */
#define CPU_FREQ      248000000

/* Type of LCD : Samsung S6B33B2 */
#define CONFIG_LCD LCD_C200

#define HAVE_MULTIDRIVE
#define NUM_DRIVES 2

#ifndef BOOTLOADER
#define HAVE_HOTSWAP
#endif /* BOOTLOADER */

#define CONFIG_USBOTG USBOTG_AS3525

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK
//#define USB_HANDLED_BY_OF
//#define USB_ENABLE_SERIAL
#define USB_VENDOR_ID 0x0781
#define USB_PRODUCT_ID 0x7452
#define HAVE_BOOTLOADER_USB_MODE
#define HAVE_USB_HID_MOUSE

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

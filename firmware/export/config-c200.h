/*
 * This config file is for the Sandisk Sansa e200
 */
#define TARGET_TREE /* this target is using the target tree system */

/* For Rolo and boot loader */
#define MODEL_NUMBER 20
#define MODEL_NAME   "Sandisk Sansa c200"

#define HW_SAMPR_CAPS       (SAMPR_CAP_44)

/* define this if you have recording possibility */
#define HAVE_RECORDING

#define REC_SAMPR_CAPS      (SAMPR_CAP_22)
#define REC_FREQ_DEFAULT    REC_FREQ_22 /* Default is not 44.1kHz */
#define REC_SAMPR_DEFAULT   SAMPR_22

/* Define bitmask of input sources - recordable bitmask can be defined
   explicitly if different */
#define INPUT_SRC_CAPS (SRC_CAP_MIC | SRC_CAP_FMRADIO)

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this if you have a light associated with the buttons */
#define HAVE_BUTTON_LIGHT

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  132
#define LCD_HEIGHT 80
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* define this if you have LCD enable function */
/* TODO: #define HAVE_LCD_ENABLE */

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
/* TODO: #define HAVE_LCD_SLEEP */

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

/* define this if you can invert the colours on your LCD */
/* TODO: #define HAVE_LCD_INVERT */

/* Define this if your LCD can set contrast */
#define HAVE_LCD_CONTRAST

#define MIN_CONTRAST_SETTING        0
#define MAX_CONTRAST_SETTING        255
#define DEFAULT_CONTRAST_SETTING    85

/* #define IRAM_LCDFRAMEBUFFER IDATA_ATTR *//* put the lcd frame buffer in IRAM */

#define CONFIG_KEYPAD SANSA_C200_PAD

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* There is no hardware tone control */
#define HAVE_SW_TONE_CONTROLS

/* The PP5024 has a built-in AustriaMicrosystems AS3514 */
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
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE 1

/* FM Tuner */
#define CONFIG_TUNER LV24020LP
#define HAVE_TUNER_PWR_CTRL

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

#define BATTERY_CAPACITY_DEFAULT 530    /* default battery capacity */
#define BATTERY_CAPACITY_MIN 530        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 530        /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0          /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

/* Hardware controlled charging? FIXME */
#define CONFIG_CHARGING CHARGING_SIMPLE

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/** Non-simulator section **/
#ifndef SIMULATOR

/* Define this if you have a PortalPlayer PP5024 */
#define CONFIG_CPU PP5022

/* Define this if you want to use the PP5024 i2c interface */
#define CONFIG_I2C I2C_PP5024

/* define this if the hardware can be powered off while charging */
/* Sansa can't be powered off while charging */
/* #define HAVE_POWEROFF_WHILE_CHARGING */

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* Define this to the CPU frequency */
#define CPU_FREQ      75000000

/* Type of LCD TODO: hopefully the same as the x5 but check this*/
#define CONFIG_LCD LCD_C200

/* Offset ( in the firmware file's header ) to the file CRC and data. These are
   only used when loading the old format rockbox.e200 file */
#define FIRMWARE_OFFSET_FILE_CRC    0x0
#define FIRMWARE_OFFSET_FILE_DATA   0x8

/* #define USB_IPODSTYLE */

#ifndef BOOTLOADER
#define HAVE_MULTIVOLUME
#define HAVE_HOTSWAP
#endif

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_ARC

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0x0781
#define USB_PRODUCT_ID 0x7450

/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define MI4_FORMAT
#define BOOTFILE_EXT    "mi4"
#define BOOTFILE        "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define ICODE_ATTR_TREMOR_NOT_MDCT

#define INCLUDE_TIMEOUT_API

#endif /* SIMULATOR */

/** Port-specific settings **/

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING     12
#define DEFAULT_BRIGHTNESS_SETTING  6

/* Default recording levels */
#define DEFAULT_REC_MIC_GAIN    23
#define DEFAULT_REC_LEFT_GAIN   23
#define DEFAULT_REC_RIGHT_GAIN  23

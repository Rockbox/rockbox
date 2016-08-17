/*
 * This config file is for the Fiio X1
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 95
#define MODEL_NAME   "Fiio X1"

#define HW_SAMPR_CAPS       SAMPR_CAP_ALL

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP
/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

#ifndef BOOTLOADER
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you have a light associated with the buttons */
#define HAVE_BUTTON_LIGHT
#define HAVE_BUTTONLIGHT_BRIGHTNESS

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* define this if you have LCD enable function */
#define HAVE_LCD_ENABLE

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_JZ4760B

#endif /* !BOOTLOADER */

/* There is no hardware tone control */
#define HAVE_SW_TONE_CONTROLS

#define CONFIG_KEYPAD FIIO_X1_PAD

/* Define this if you can detect headphones */
#define HAVE_HEADPHONE_DETECTION

/* Define this to enable morse code input */
#define HAVE_MORSE_INPUT

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* Define this to select the audio codec */
#define HAVE_PCM5142_CODEC

/* LCD dimensions */
#define LCD_WIDTH  320
#define LCD_HEIGHT 240
/* sqrt(320^2 + 240^2) / 2.0 = 140.9 */
#define LCD_DPI 141
#define LCD_DEPTH  16   /* 65536 colours */
#define LCD_PIXELFORMAT RGB565 /* rgb565 */

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      0
#define MAX_BRIGHTNESS_SETTING      100
#define DEFAULT_BRIGHTNESS_SETTING  50

/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* define this if you have a flash memory storage */
#define HAVE_FLASH_STORAGE

/* define this if the flash memory uses the SecureDigital Memory Card protocol */
#define CONFIG_STORAGE STORAGE_SD
#define NUM_DRIVES 1
#define HAVE_HOTSWAP
#define HAVE_HOTSWAP_AS_MAIN

/* todo */
#define BATTERY_CAPACITY_DEFAULT 1700   /* default battery capacity */
#define BATTERY_CAPACITY_MIN 550        /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 2500       /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 100        /* capacity increment */
#define BATTERY_TYPES_COUNT  1          /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Charging implemented by hardware */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

/* Define this if you have an IMX233*/
#define CONFIG_CPU JZ4760B

/* Define this if you want to use the IMX233 i2c interface */
#define CONFIG_I2C I2C_JZ4760B

/* define current usage levels (based on battery bench) */
#define CURRENT_NORMAL     35
#define CURRENT_BACKLIGHT  30
#define CURRENT_RECORD     CURRENT_NORMAL

/* maximum charging current */
#define CURRENT_MAX_CHG   200

/* Define this to the CPU frequency */
#define CPU_FREQ      454000000

/* Type of LCD */
#define CONFIG_LCD LCD_FIIOX1

/* Offset ( in the firmware file's header ) to the file CRC and data. These are
   only used when loading the old format rockbox.e200 file */
#define FIRMWARE_OFFSET_FILE_CRC    0x0
#define FIRMWARE_OFFSET_FILE_DATA   0x8

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_JZ4760

/* enable these for the usb stack */
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0x041e
#define USB_PRODUCT_ID 0x2020
#define HAVE_USB_HID_MOUSE
#define HAVE_BOOTLOADER_USB_MODE

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

#define BOOTFILE_EXT    "fiio"
#define BOOTFILE        "rockbox." BOOTFILE_EXT
#define BOOTDIR         "/.rockbox"

#define INCLUDE_TIMEOUT_API
 

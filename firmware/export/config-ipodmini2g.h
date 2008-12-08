/*
 * This config file is for the Apple iPod Mini 2nd Gen
 */
#define TARGET_TREE /* this target is using the target tree system */

#define IPOD_ARCH 1

#define MODEL_NAME   "Apple iPod Mini 2g"

/* For Rolo and boot loader */
#define MODEL_NUMBER 11

/* define this if you use an ATA controller */
#define CONFIG_STORAGE STORAGE_ATA

/* define this if you have recording possibility */
/*#define HAVE_RECORDING*/

/* define the bitmask of hardware sample rates */
#define HW_SAMPR_CAPS   (SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_48 | \
                         SAMPR_CAP_44 | SAMPR_CAP_32 | SAMPR_CAP_8)

/* define the bitmask of recording sample rates
#define REC_SAMPR_CAPS  (SAMPR_CAP_96 | SAMPR_CAP_88 | SAMPR_CAP_48 | \
                         SAMPR_CAP_44 | SAMPR_CAP_32 | SAMPR_CAP_8) */

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this if you can invert the colours on your LCD */
#define HAVE_LCD_INVERT

/* define this if the LCD needs to be shutdown */
#define HAVE_LCD_SHUTDOWN

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN
/* define this if you have access to the pitchscreen */
#define HAVE_PITCHSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  138
#define LCD_HEIGHT 110
#define LCD_DEPTH  2   /* 4 colours - 2bpp */

#define LCD_PIXELFORMAT HORIZONTAL_PACKING

#define HAVE_LCD_CONTRAST

/* LCD contrast */
#define MIN_CONTRAST_SETTING        5
#define MAX_CONTRAST_SETTING        63
#define DEFAULT_CONTRAST_SETTING    40 /* Match boot contrast */

/* define this if you can flip your LCD */
#define HAVE_LCD_FLIP

#define CONFIG_KEYPAD IPOD_4G_PAD

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_PCF50605

/* Define if the device can wake from an RTC alarm */
#define HAVE_RTC_ALARM

/* Define this if you can switch on/off the accessory power supply */
#define HAVE_ACCESSORY_SUPPLY

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

/* Define this if you have the WM8711 audio codec */
#define HAVE_WM8711

/* WM8721 has no tone controls, so we use the software ones */
#define HAVE_SW_TONE_CONTROLS

#define AB_REPEAT_ENABLE 1
#define ACTION_WPSAB_SINGLE ACTION_WPS_BROWSE

/* define this if you have a disk storage, i.e. something
   that needs spinups and can cause skips when shaked */
#define HAVE_DISK_STORAGE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT

/* We can fade the backlight by using PWM */
#define HAVE_BACKLIGHT_PWM_FADING

/* define this if the unit uses a scrollwheel for navigation */
#define HAVE_SCROLLWHEEL
/* define from which rotation speed [degree/sec] on the acceleration starts */
#define WHEEL_ACCEL_START 270
/* define type of acceleration (1 = ^2, 2 = ^3, 3 = ^4) */
#define WHEEL_ACCELERATION 3

/* Define this if you can detect headphones */
#define HAVE_HEADPHONE_DETECTION

#define BATTERY_CAPACITY_DEFAULT 400 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 400 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 800 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50  /* capacity increment */
#define BATTERY_TYPES_COUNT  1   /* only one type */

/* Hardware controlled charging? */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define this if the unit can be powered or charged via USB */
#define HAVE_USB_POWER

#ifndef SIMULATOR

/* Define this if you have a PortalPlayer PP5022 */
#define CONFIG_CPU PP5022

/* Define this if you want to use the PP5020 i2c interface */
#define CONFIG_I2C I2C_PP5020

/* We're able to shut off power to the HDD */
#define HAVE_ATA_POWER_OFF

/* define this if the hardware can be powered off while charging */
//#define HAVE_POWEROFF_WHILE_CHARGING

/* The start address index for ROM builds */
#define ROM_START 0x00000000

/* The size of the flash ROM */
#define FLASH_SIZE 0x100000

/* Define this to the CPU frequency */
#define CPU_FREQ      11289600

#define CONFIG_LCD LCD_IPODMINI

/* Offset ( in the firmware file's header ) to the file length */
#define FIRMWARE_OFFSET_FILE_LENGTH 0

/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define USB_IPODSTYLE

/* USB On-the-go */
#define CONFIG_USBOTG USBOTG_ARC

/* enable these for the experimental usb stack */
#define HAVE_USBSTACK
#define USB_VENDOR_ID 0x05ac
#define USB_PRODUCT_ID 0x1205


/* Virtual LED (icon) */
#define CONFIG_LED LED_VIRTUAL

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

/* Define this if you can read an absolute wheel position */
#define HAVE_WHEEL_POSITION

#define BOOTFILE_EXT "ipod"
#define BOOTFILE "rockbox." BOOTFILE_EXT
#define BOOTDIR "/.rockbox"

#define ICODE_ATTR_TREMOR_NOT_MDCT

#endif

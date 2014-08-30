/*
 * This config file is for Rockbox as an application on Android
 */

/* We don't run on hardware directly */
#define CONFIG_PLATFORM (PLATFORM_HOSTED|PLATFORM_ANDROID)

/* For Rolo and boot loader */
#define MODEL_NUMBER 95

#define MODEL_NAME   "iBasso DX90"

#define USB_NONE

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* define this if you want album art for this target */
#define HAVE_ALBUMART

/* define this to enable bitmap scaling */
#define HAVE_BMP_SCALING

/* define this to enable JPEG decoding */
#define HAVE_JPEG

/* define this if you have access to the quickscreen */
#define HAVE_QUICKSCREEN

/* define this if you would like tagcache to build on this target */
#define HAVE_TAGCACHE

/* LCD dimensions */
#define LCD_WIDTH  320
#define LCD_HEIGHT 240
#define LCD_DPI 166
#define LCD_DEPTH  16
#define LCD_PIXELFORMAT RGB565

#define HAVE_LCD_ENABLE

/* define this to indicate your device's keypad */
#define HAVE_TOUCHSCREEN
#define HAVE_BUTTON_DATA

/* define this if you have RTC RAM available for settings */
//#define HAVE_RTC_RAM

/* define this if you have a real-time clock */
//#define CONFIG_RTC APPLICATION

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x160000

#define AB_REPEAT_ENABLE

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      4
#define MAX_BRIGHTNESS_SETTING      255
#define DEFAULT_BRIGHTNESS_SETTING  255

/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

//#define HAVE_MULTIMEDIA_KEYS
#define CONFIG_KEYPAD DX50_PAD

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* define this if the host platform can change volume outside of rockbox */
//#define PLATFORM_HAS_VOLUME_CHANGE

#define HAVE_SW_TONE_CONTROLS

#define HAVE_SW_VOLUME_CONTROL

#define BATTERY_CAPACITY_DEFAULT 2100 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1700 /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 3200 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 50   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

#define CONFIG_CHARGING CHARGING_SIMPLE

#define NO_LOW_BATTERY_SHUTDOWN

/* Define current usage levels. */
#define CURRENT_NORMAL     210 /* 10 hours from a 2100 mAh battery */
#define CURRENT_BACKLIGHT  30 /* TBD */
#define CURRENT_RECORD     0  /* no recording */

/* Define this to the CPU frequency */
/*
#define CPU_FREQ 48000000
*/

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING


/* Offset ( in the firmware file's header ) to the file CRC */
#define FIRMWARE_OFFSET_FILE_CRC 0

/* Offset ( in the firmware file's header ) to the real data */
#define FIRMWARE_OFFSET_FILE_DATA 8

#define CONFIG_LCD LCD_COWOND2

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

#define BOOTDIR "/.rockbox"

/* No special storage */
#define CONFIG_STORAGE STORAGE_HOSTFS
#define HAVE_STORAGE_FLUSH


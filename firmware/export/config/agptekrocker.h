/*
 * This config file is for the Agptek Rocket
 */

/* For Rolo and boot loader */
#define MODEL_NUMBER 103//???

#define MODEL_NAME   "Agptek Rocker"

/* LCD dimensions */
#define LCD_WIDTH  128
#define LCD_HEIGHT 160
/* sqrt(128^2 + 160^2) / 2 = 102. */
#define LCD_DPI 102

#ifndef SIMULATOR
#define CONFIG_PLATFORM (PLATFORM_HOSTED)
#endif

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

#define HAVE_LCD_ENABLE

/* Define this if the LCD can shut down */
#define HAVE_LCD_SHUTDOWN

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

#define LCD_DEPTH  32
/* Check that but should not matter */
#define LCD_PIXELFORMAT XRGB8888

#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults: the backlight driver
 * has levels from 0 to 2555. But 0 is off so start at 1.
 */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      255
#define DEFAULT_BRIGHTNESS_SETTING  70

/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* define this if you have a real-time clock */
#define CONFIG_RTC APPLICATION

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x100000

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC
#define HAVE_ROCKER_CODEC

#define HAVE_HEADPHONE_DETECTION

/* KeyPad configuration for plugins */
#define CONFIG_KEYPAD AGPTEK_ROCKER_PAD

#ifndef SIMULATOR
/* We have usb power and can detect usb but it is handled by Linux */
#define HAVE_USB_POWER

#endif

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Linux controlls charging, we can monitor */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* same dimensions as gigabeats */
#define CONFIG_LCD LCD_INGENIC_LINUX

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Define this to the CPU frequency */
#define CPU_FREQ            532000000

/* No special storage */
#define CONFIG_STORAGE (STORAGE_HOSTFS)//|STORAGE_SD)
//#define MULTIDRIVE_DIR "/mnt/sd_0"
//#define NUM_DRIVES 1
//#define HAVE_HOTSWAP
#define HAVE_STORAGE_FLUSH

/* Battery */
#define BATTERY_TYPES_COUNT  1

/* Audio codec */
#define HAVE_ROCKER_CODEC

/* We don't have hardware controls */
#define HAVE_SW_TONE_CONTROLS

/* Battery */
#define BATTERY_CAPACITY_DEFAULT 600 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 600  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 600 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */

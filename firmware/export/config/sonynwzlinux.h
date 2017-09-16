/*
 * This config file is for the Sony NWZ Linux based targets
 */

#ifndef SIMULATOR
#define CONFIG_PLATFORM (PLATFORM_HOSTED)
#endif

/* define this if you have a bitmap LCD display */
#define HAVE_LCD_BITMAP

/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

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

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

#define LCD_DEPTH  16
/* Check that but should not matter */
#define LCD_PIXELFORMAT RGB565

#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults: the backlight driver only
 * has levels from 0 to 5. But 0 is off so start at 1. The driver has configurable
 * periods for fade in/out but we can't easily export that to Rockbox */
#define MIN_BRIGHTNESS_SETTING      1
#define MAX_BRIGHTNESS_SETTING      5
#define DEFAULT_BRIGHTNESS_SETTING  4

/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* define this if you have a real-time clock */
#define CONFIG_RTC 0

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x100000

/* Define this if you do software codec */
#define CONFIG_CODEC SWCODEC

/* KeyPad configuration for plugins */
#define CONFIG_KEYPAD SONY_NWZ_PAD
#define HAS_BUTTON_HOLD

/** Non-simulator section **/
#ifndef SIMULATOR
/* We have usb power and can detect usb but it is handled by Linux */
#define HAVE_USB_POWER
#define USB_NONE

/* Audio codec */
#define HAVE_NWZ_LINUX_CODEC

#endif /* SIMULATOR */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Linux controlls charging, we can monitor */
#define CONFIG_CHARGING CHARGING_MONITOR

/* define this if the hardware can be powered off while charging */
#define HAVE_POWEROFF_WHILE_CHARGING

/* same dimensions as gigabeats */
#define CONFIG_LCD LCD_NWZ_LINUX

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Define this to the CPU frequency */
#define CPU_FREQ            532000000

/* No special storage */
#define CONFIG_STORAGE STORAGE_HOSTFS
#define HAVE_STORAGE_FLUSH

/* Battery */
#define BATTERY_TYPES_COUNT  1

/* special define to be use in various places */
#define SONY_NWZ_LINUX

/* Battery */
#define BATTERY_CAPACITY_DEFAULT 600 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 600  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 600 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */

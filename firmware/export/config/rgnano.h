/*
 * This config file is for the Anbernic RG Nano
 */

/* We don't run on hardware directly */
#ifndef SIMULATOR
#define CONFIG_PLATFORM (PLATFORM_HOSTED)
#define PIVOT_ROOT "/mnt"
#endif

#define HAVE_FPU

/* For Rolo and boot loader */
#define MODEL_NUMBER 100

#define MODEL_NAME   "Anbernic RG Nano"

#ifndef SIMULATOR
#define USB_NONE
#endif

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
#define LCD_WIDTH  240
#define LCD_HEIGHT 240
#define LCD_DEPTH  16
#define LCD_PIXELFORMAT RGB565

/* define this to indicate your device's keypad */
#define HAVE_BUTTON_DATA
#define HAVE_VOLUME_IN_LIST

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_HOSTED

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x200000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x200000

#define AB_REPEAT_ENABLE

/* Battery stuff */
#define CONFIG_BATTERY_MEASURE (PERCENTAGE_MEASURE|VOLTAGE_MEASURE|CURRENT_MEASURE)
#define CONFIG_CHARGING CHARGING_MONITOR
#define HAVE_POWEROFF_WHILE_CHARGING
#define BATTERY_DEV_NAME "axp20x-battery"
#define POWER_DEV_NAME "axp20x-usb"

#define BATTERY_CAPACITY_DEFAULT 1050 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 1050  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 1050 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */

/* Voltage reported in millivolts */
#define BATTERY_VOLTAGE_SCALE_MUL 1
#define BATTERY_VOLTAGE_SCALE_DIV 1

/* Define this for LCD backlight available */
#define BACKLIGHT_RG_NANO
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      0
#define MAX_BRIGHTNESS_SETTING      11
#define DEFAULT_BRIGHTNESS_SETTING  4

#define CONFIG_KEYPAD RG_NANO_PAD

/* Use SDL audio/pcm in a SDL app build */
#define HAVE_SDL
#define HAVE_SDL_AUDIO

#define HAVE_SW_TONE_CONTROLS

/* Define this to the CPU frequency */
/*
#define CPU_FREQ 48000000
*/

#define CONFIG_LCD LCD_COWOND2

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

#define BOOTDIR "/.rockbox"

/* No special storage */
#define CONFIG_STORAGE STORAGE_HOSTFS
#define HAVE_STORAGE_FLUSH

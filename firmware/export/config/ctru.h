/*
 * This config file is for the N3DS hosted application
 */

/* We don't run on hardware directly */
#define CONFIG_PLATFORM (PLATFORM_HOSTED|PLATFORM_CTRU)
#define HAVE_FPU

/* For Rolo and boot loader */
#define MODEL_NUMBER  100s
#define MODEL_NAME    "CTRU"

#define USB_NONE

#define CONFIG_CPU    N10480H

#define CPU_FREQ      268000000

/* Define this if you have adjustable CPU frequency */
/* #define HAVE_ADJUSTABLE_CPU_FREQ */

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

#define LCD_DEPTH  16
#define LCD_PIXELFORMAT RGB565

#define LCD_OPTIMIZED_UPDATE
#define LCD_OPTIMIZED_UPDATE_RECT
#define LCD_OPTIMIZED_BLIT_YUV

/* define this to indicate your device's keypad */
#define HAVE_TOUCHSCREEN
#define HAVE_BUTTON_DATA

/* define this if you have a real-time clock */
#define CONFIG_RTC APPLICATION

/* Power management */
#define CONFIG_BATTERY_MEASURE PERCENTAGE_MEASURE
#define CONFIG_CHARGING        CHARGING_MONITOR
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE


/* #define HAVE_SCROLLWHEEL */
#define CONFIG_KEYPAD CTRU_PAD

#define HAVE_CTRU_AUDIO
#define HAVE_HEADPHONE_DETECTION
/* #define HAVE_SW_VOLUME_CONTROL */
/* #define PCM_SW_VOLUME_UNBUFFERED */
/* #define PCM_SW_VOLUME_FRACBITS  (16) */

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      16
#define MAX_BRIGHTNESS_SETTING      142
#define BRIGHTNESS_STEP             5
#define DEFAULT_BRIGHTNESS_SETTING  28
#define CONFIG_BACKLIGHT_FADING     BACKLIGHT_FADING_SW_SETTING

#define CONFIG_LCD LCD_COWOND2

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

#define BOOTDIR "/"

/* No special storage */
#define CONFIG_STORAGE STORAGE_HOSTFS
#define HAVE_STORAGE_FLUSH


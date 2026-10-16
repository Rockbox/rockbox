/*
 * This config file is for the SDL application
 */

/* We don't run on hardware directly */
#define CONFIG_PLATFORM (PLATFORM_HOSTED|PLATFORM_GAME_CONSOLE)
#define HAVE_FPU

/* For Rolo and boot loader */
#define MODEL_NUMBER 127
#define MODEL_NAME   "PSP"

#define USB_NONE

#define CONFIG_CPU    TACHYON

/* Define this to the CPU frequency */
#define CPU_FREQ      222000000

/* Define this if you have adjustable CPU frequency */
#define HAVE_ADJUSTABLE_CPU_FREQ

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
#define LCD_WIDTH  480
#define LCD_HEIGHT 272

#define LCD_DEPTH  32
#define LCD_PIXELFORMAT XRGB8888

/* Define this if your LCD can be enabled/disabled */
#define HAVE_LCD_ENABLE

/* Define this if your LCD can be put to sleep. HAVE_LCD_ENABLE
   should be defined as well. */
//#define HAVE_LCD_SLEEP

#ifndef SIMULATOR
#define PIVOT_ROOT "ms0:"
#endif

/* The PSP supports 44 kHz and 48 kHz natively
 */
#define HW_SAMPR_CAPS (SAMPR_CAP_44 | SAMPR_CAP_48)

/* define the bitmask of recording sample rates */
/* #define REC_SAMPR_CAPS HW_SAMPR_CAPS */
/* #define HAVE_RECORDING */

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_HOSTED

/* Power management */
#define CONFIG_BATTERY_MEASURE PERCENTAGE_MEASURE
#define CONFIG_CHARGING        CHARGING_MONITOR
#define HAVE_POWEROFF_WHILE_CHARGING
#define HAVE_SW_POWEROFF

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x100000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x80000

#define AB_REPEAT_ENABLE

#define CONFIG_KEYPAD PSP_PAD

#define HAVE_PSP_AUDIO
#define HAVE_WM8973G_CODEC
#define HAVE_HEADPHONE_DETECTION
/* #define HAVE_SW_VOLUME_CONTROL */
/* #define PCM_SW_VOLUME_UNBUFFERED */
/* #define PCM_SW_VOLUME_FRACBITS  (16) */

/* Define this for LCD backlight available */
#define HAVE_BACKLIGHT
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
#define MIN_BRIGHTNESS_SETTING      0
#define MAX_BRIGHTNESS_SETTING      100
#define BRIGHTNESS_STEP             10
#define DEFAULT_BRIGHTNESS_SETTING  70
#define CONFIG_BACKLIGHT_FADING     BACKLIGHT_FADING_SW_SETTING

#define CONFIG_LCD LCD_COWOND2

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

#define BOOTDIR "/.rockbox"

/* No special storage */
#define CONFIG_STORAGE STORAGE_HOSTFS
#define HAVE_STORAGE_FLUSH

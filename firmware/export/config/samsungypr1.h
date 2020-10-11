/*
 * This config file is for the RockBox as application on the Samsung YP-R1 player.
 * The target name for ifdefs is: SAMSUNG_YPR1; or CONFIG_PLATFORM & PLAFTORM_YPR1
 */

/* We don't run on hardware directly */
/* YP-R1 need it too of course */
#ifndef SIMULATOR
#define CONFIG_PLATFORM (PLATFORM_HOSTED)
#define PIVOT_ROOT "/mnt/media0"
#endif

/* For Rolo and boot loader */
#define MODEL_NUMBER 101

#define MODEL_NAME   "Samsung YP-R1"




/* define this if you have a colour LCD */
#define HAVE_LCD_COLOR

/* Define this if the LCD can shut down */
#define HAVE_LCD_SHUTDOWN

/* define this if you have LCD enable function */
#define HAVE_LCD_ENABLE

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

/* We try to support both orientation, looking forward for a future dynamic switch */
#define CONFIG_ORIENTATION SCREEN_PORTRAIT

/* LCD dimensions */
#if CONFIG_ORIENTATION == SCREEN_PORTRAIT
#define LCD_WIDTH  240
#define LCD_HEIGHT 400
#else
#define LCD_WIDTH  400
#define LCD_HEIGHT 240
#endif

#define LCD_DEPTH  24
/* Calculated value, important for touch sensor */
#define LCD_DPI    180
/* Check that but should not matter */
#define LCD_PIXELFORMAT RGB888

/* Capacitive touchscreen */
#define HAVE_TOUCHSCREEN
#define HAVE_BUTTON_DATA

/* We have backlight to control */
#define HAVE_BACKLIGHT

/* Define this for LCD backlight brightness available */
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
/* From 0 to 32. Default is 18, that is set by the bootloader
 * We stay a little lower since OF makes a distinction between
 * two LCD screens (there is no reason to go further than 25 in any case)
 */
#define MIN_BRIGHTNESS_SETTING          1
#define MAX_BRIGHTNESS_SETTING          25
#define DEFAULT_BRIGHTNESS_SETTING      18

/* Which backlight fading type? */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x100000

#define AB_REPEAT_ENABLE




/* R1 KeyPad configuration for plugins */
#define CONFIG_KEYPAD SAMSUNG_YPR1_PAD
#define BUTTON_DRIVER_CLOSE

/* We have WM1808, which so far is compatible with the following */
#define HAVE_WM8978

/* For the moment the only supported frequency is 44kHz,
 * even if the codec supports more (see wmcodec-ypr1.c)
 */
#define HW_SAMPR_CAPS SAMPR_CAP_44

/* define the bitmask of recording sample rates */
#define REC_SAMPR_CAPS HW_SAMPR_CAPS /* Same as playback */


/* R1 has a standard linux RTC driver on /dev/rtc1 (->/dev/rtc)
 * The RTC is S35392 A
 */
#define CONFIG_RTC APPLICATION

#define BATTERY_CAPACITY_DEFAULT 600 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 600  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 600 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

/* R1's fuel gauge max17040 can supply both kind of values */
#define CONFIG_BATTERY_MEASURE PERCENTAGE_MEASURE

/* Hardware controls charging, we can monitor */
#define CONFIG_CHARGING CHARGING_MONITOR

#define CONFIG_LCD LCD_SAMSUNGYPR1

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* define this if the target has volume keys which can be used in the lists */
#define HAVE_VOLUME_IN_LIST

/* This mount point resides in the rootfs, binded to /mnt/media0/.rockbox */
#define BOOTDIR "/.rockbox"

/* No special storage */
#define CONFIG_STORAGE STORAGE_HOSTFS
#define HAVE_STORAGE_FLUSH

/** Non-simulator section **/
#ifndef SIMULATOR

#define USB_NONE

/* We have the Si4709 */
#define CONFIG_TUNER SI4700
#define HAVE_TUNER_PWR_CTRL
#define HAVE_RDS_CAP

/* Define this for FM radio input available */
#define HAVE_FMRADIO_IN
#define INPUT_SRC_CAPS SRC_CAP_FMRADIO

/* We have a GPIO pin that detects it */
#define HAVE_HEADPHONE_DETECTION

/* Define current usage levels. */
#define CURRENT_NORMAL     24 /* ~25h, on 600mAh that's about 24mA */
#define CURRENT_BACKLIGHT  62 /* ~6,5h -> 92mA. Minus 24mA normal that gives us 68mA */

#endif /* SIMULATOR */

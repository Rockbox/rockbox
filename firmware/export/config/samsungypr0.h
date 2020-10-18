/*
 * This config file is for the RockBox as application on the Samsung YP-R0 player.
 * The target name for ifdefs is: SAMSUNG_YPR0; or CONFIG_PLATFORM & PLAFTORM_YPR0
 */

/* We don't run on hardware directly */
/* YP-R0 need it too of course */
#ifndef SIMULATOR
#define CONFIG_PLATFORM (PLATFORM_HOSTED)
#define PIVOT_ROOT "/mnt/media0"
#endif

/* For Rolo and boot loader */
#define MODEL_NUMBER 100

#define MODEL_NAME   "Samsung YP-R0"




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

/* LCD dimensions */
#define LCD_WIDTH  240
#define LCD_HEIGHT 320
/* sqrt(240^2 + 320^2) / 2.6 = 153.8 */
#define LCD_DPI 154

#define LCD_DEPTH  24
/* Check that but should not matter */
#define LCD_PIXELFORMAT RGB888

/* YP-R0 has the backlight */
#define HAVE_BACKLIGHT

/* Define this for LCD backlight brightness available */
#define HAVE_BACKLIGHT_BRIGHTNESS

/* Main LCD backlight brightness range and defaults */
/* 0 is turned off. 31 is the real maximum for the ASCODEC DCDC but samsung doesn't use any value over 15, so it safer to don't go up too much */
#define MIN_BRIGHTNESS_SETTING          1
#define MAX_BRIGHTNESS_SETTING          15
#define DEFAULT_BRIGHTNESS_SETTING      4

/* Which backlight fading type? */
/* TODO: ASCODEC has an auto dim feature, so disabling the supply to leds should do the trick. But for now I tested SW fading only */
#define CONFIG_BACKLIGHT_FADING BACKLIGHT_FADING_SW_SETTING

/* define this if you have RTC RAM available for settings */
/* TODO: in theory we could use that, ascodec offers us such a ram. we have also a small device, part of the nand of 1 MB size, that Samsung uses to store region code etc and it's almost unused space */
//#define HAVE_RTC_RAM

/* define this if you have a real-time clock */
#define CONFIG_RTC RTC_AS3514
#define HAVE_RTC_ALARM

/* The number of bytes reserved for loadable codecs */
#define CODEC_SIZE 0x80000

/* The number of bytes reserved for loadable plugins */
#define PLUGIN_BUFFER_SIZE 0x100000

#define AB_REPEAT_ENABLE
#define ACTION_WPSAB_SINGLE ACTION_WPS_HOTKEY




/* R0 KeyPad configuration for plugins */
#define CONFIG_KEYPAD SAMSUNG_YPR0_PAD
#define BUTTON_DRIVER_CLOSE

/** Non-simulator section **/
#ifndef SIMULATOR

/*TODO: implement USB data transfer management -> see safe mode script and think a way to implemtent it in the code */
#define USB_NONE

/* The YPR0 has a as3534 codec */
#define HAVE_AS3514
#define HAVE_AS3543

/* We don't have hardware controls */
#define HAVE_SW_TONE_CONTROLS

/* We have the Si4709, which supports RDS */
#define CONFIG_TUNER SI4700
#define HAVE_TUNER_PWR_CTRL
#define HAVE_RDS_CAP

/* Define this for FM radio input available */
#define HAVE_FMRADIO_IN
#define INPUT_SRC_CAPS SRC_CAP_FMRADIO

/* We have a GPIO pin that detects this */
#define HAVE_HEADPHONE_DETECTION

/* Define current usage levels. */
#define CURRENT_NORMAL     24 /* ~25h, on 600mAh that's about 24mA */
#define CURRENT_BACKLIGHT  62 /* ~6,5h -> 92mA. Minus 24mA normal that gives us 68mA */

#endif /* SIMULATOR */

#define BATTERY_CAPACITY_DEFAULT 600 /* default battery capacity */
#define BATTERY_CAPACITY_MIN 600  /* min. capacity selectable */
#define BATTERY_CAPACITY_MAX 600 /* max. capacity selectable */
#define BATTERY_CAPACITY_INC 0   /* capacity increment */
#define BATTERY_TYPES_COUNT  1    /* only one type */

#define CONFIG_BATTERY_MEASURE VOLTAGE_MEASURE

/* Linux controlls charging, we can monitor */
#define CONFIG_CHARGING CHARGING_MONITOR

/* We want to be able to reset the averaging filter */
#define HAVE_RESET_BATTERY_FILTER

/* same dimensions as gigabeats */
#define CONFIG_LCD LCD_YPR0

/* Define this if a programmable hotkey is mapped */
#define HAVE_HOTKEY

/* Define this if you have a software controlled poweroff */
#define HAVE_SW_POWEROFF

/* Define this if you have adjustable CPU frequency
 * NOTE: We could do that on this device, but it's probably better
 * to let linux do it (we set ondemand governor before loading Rockbox) */
/* #define HAVE_ADJUSTABLE_CPU_FREQ */
/* Define this to the CPU frequency */
#define CPU_FREQ            532000000
/* 0.8Vcore using 200 MHz */
/* #define CPUFREQ_DEFAULT     200000000 */
/* This is 400 MHz -> not so powersaving-ful */
/* #define CPUFREQ_NORMAL      400000000 */
/* Max IMX37 Cpu Frequency */
/* #define CPUFREQ_MAX         CPU_FREQ */

/* This folder resides in the ReadOnly CRAMFS. It is binded to /mnt/media0/.rockbox */
#define BOOTDIR "/.rockbox"

/* External SD card can be mounted */
#define CONFIG_STORAGE (STORAGE_HOSTFS|STORAGE_SD)
#define HAVE_MULTIDRIVE  /* But _not_ CONFIG_STORAGE_MULTI */
#define NUM_DRIVES 2
#define HAVE_HOTSWAP
#define HAVE_STORAGE_FLUSH
#define MULTIDRIVE_DIR "/mnt/mmc"
#define MULTIDRIVE_DEV "/sys/block/mmcblk0"
